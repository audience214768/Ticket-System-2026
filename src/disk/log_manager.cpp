#include "disk/log_manager.h"
#include "common/config.h"
#include "disk/disk_manager.h"
#include "page/b_plus_tree_page.h"
#include "shared_ptr/shared_ptr.hpp"
#include <cstddef>
#include <mutex>

using std::unique_lock;

void LogManager::StartFlushLog() {
    while (true) {
        unique_lock<mutex> lock(log_mutex_);
        char tmp[BUFFER_SIZE];
        cv_consume_.wait(lock, [this] { return stop_ || offset_ > 0; });
        if (stop_ && offset_ == 0) {
            break;
        }
        if (offset_ > 0) {
            size_t size = offset_;
            size_t last_lsn = lsn_ - 1;
            memcpy(tmp, buffer, offset_);
            offset_ = 0;
            lock.unlock();
            cv_produce_.notify_all();
            file_.write(tmp, offset_);
            file_.flush();
            flushed_lsn_ = last_lsn;
        }
    }
}

LogManager::LogManager(const vector<shared_ptr<DiskManager>> &disk_manager) : stop_(false) {
    flusher_ = thread([&] { StartFlushLog(); });
    file_.open("log", ios::in | ios::out | ios::app | ios::binary);
    if (!file_.is_open()) {
        file_.clear();
        file_.open("log", ios::out | ios::app | ios::binary);
    } else {
        Recover();
    }
}

void LogManager::Recover() {
    vector<char> data(DISK_PAGE_SIZE);
    page_id_t page_id;
    size_t lsn;
    file_.seekg(0, ios::end);
    auto offset = file_.tellg();
    while (offset > 0) {
        offset -= BUFFER_SIZE;
        file_.seekg(offset);
        file_.read(reinterpret_cast<char *>(page_id), sizeof(page_id_t));
        file_.read(reinterpret_cast<char *>(&lsn), sizeof(size_t));
        disk_manager_[page_id >> FILE_BIT]->ReadPage(page_id, data.data());
        BPlusTreePage *page = reinterpret_cast<BPlusTreePage *>(data.data());
        if (page->GetLSN() < lsn) {
            page->SetLSN(lsn);
            file_.read(data.data(), DISK_PAGE_SIZE);
            disk_manager_[page_id >> FILE_BIT]->WritePage(page_id, data.data());
        }
    }
}

size_t LogManager::AppendLog(page_id_t page_id, const char *data) {
    unique_lock<mutex> lock(log_mutex_);
    cv_consume_.notify_all();
    cv_produce_.wait(lock, [this] { return offset_ + LOG_SIZE < BUFFER_SIZE; });
    size_t lsn = lsn_++;
    memcpy(buffer + offset_, &page_id, sizeof(page_id));
    offset_ += sizeof(page_id);
    memcpy(buffer + offset_, &lsn, sizeof(size_t));
    offset_ += sizeof(size_t);
    memcpy(buffer + offset_, data, DISK_PAGE_SIZE);
    offset_ += DISK_PAGE_SIZE;
    return lsn;
}

void LogManager::FlushToLsn(size_t lsn) {
    unique_lock<mutex> lock(log_mutex_);
    cv_consume_.notify_all();
    cv_produce_.wait(lock, [this, lsn] { return flushed_lsn_ >= lsn; });
}

LogManager::~LogManager() {
    {
        unique_lock<mutex> lock(log_mutex_);
        stop_ = true;
    }
    cv_consume_.notify_all();
    if (flusher_.joinable()) {
        flusher_.join();
    }
}