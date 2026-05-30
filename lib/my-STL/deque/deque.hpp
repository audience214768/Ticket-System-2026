#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include <strings.h>
#include "exceptions.hpp"

namespace sjtu { 

template<class T>
class deque {
  static constexpr int BlockSize = 16;
public:
	class const_iterator;
	class iterator {
    friend const_iterator;
	private:
		/**
		 * TODO add data members
		 *   just add whatever you want.
		 */
    deque *ptr_ = nullptr;
    int pos_ = 0;
	public:
		/**
		 * return a new iterator which pointer n-next elements
		 *   even if there are not enough elements, the behaviour is **undefined**.
		 * as well as operator-
		 */
    iterator() = default;
    iterator(deque *ptr, const int &pos):ptr_(ptr), pos_(pos) {}
		iterator operator+(const int &n) const {
			//TODO
      return iterator(ptr_, pos_ + n);
		}
		iterator operator-(const int &n) const {
			//TODO
      return iterator(ptr_, pos_ - n);
		}
		// return th distance between two iterator,
		// if these two iterators points to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const {
			//TODO
      if (ptr_ != rhs.ptr_) {
        throw invalid_iterator();
      }
      return pos_ - rhs.pos_;
		}
		iterator operator+=(const int &n) {
			//TODO
      pos_ += n;
      return *this;
		}
		iterator operator-=(const int &n) {
			//TODO
      pos_ -= n;
      return *this;
		}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
      iterator tmp(ptr_, pos_);
      pos_++;
      return tmp;
    }
		/**
		 * TODO ++iter
		 */
		iterator& operator++() {
      pos_++;
      return *this;
    }
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
      iterator tmp(ptr_, pos_);
      pos_--;
      return tmp;
    }
		/**
		 * TODO --iter
		 */
		iterator& operator--() {
      pos_--;
      return *this;
    }
		/**
		 * TODO *it
		 */
		T& operator*() const {
      return (*ptr_)[pos_];
    }
		/**
		 * TODO it->field
		 */
		T* operator->() const noexcept {
      return &(*ptr_)[pos_];
    }
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory).
		 */
		bool operator==(const iterator &rhs) const {
      return ptr_ == rhs.ptr_ && pos_ == rhs.pos_;
    }
		bool operator==(const const_iterator &rhs) const {
      return ptr_ == rhs.ptr_ && pos_ == rhs.pos_;
    }
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
      return ptr_ != rhs.ptr_ || pos_ != rhs.pos_;
    }
		bool operator!=(const const_iterator &rhs) const {
      return ptr_ != rhs.ptr_ || pos_ != rhs.pos_;
    }
	};
	class const_iterator {
		// it should has similar member method as iterator.
		//  and it should be able to construct from an iterator.
    friend iterator;
		private:
			// data members.
      const deque *ptr_ = nullptr;
      int pos_ = 0;
		public:
			const_iterator() = default;
      const_iterator(const deque *ptr, const int &pos):ptr_(ptr), pos_(pos) {}
			const_iterator(const iterator &other):ptr_(other.ptr_), pos_(other.pos_) {
				// TODO
			}
			const_iterator operator+(const int &n) const {
				//TODO
        return const_iterator(ptr_, pos_ + n);
			}
			const_iterator operator-(const int &n) const {
				//TODO
        return const_iterator(ptr_, pos_ - n);
			}
      int operator-(const const_iterator &rhs) const {
			  //TODO
        if (ptr_ != rhs.ptr_) {
          throw invalid_iterator();
        }
        return pos_ - rhs.pos_;
		  }
		  const_iterator operator+=(const int &n) {
			  //TODO
        pos_ += n;
        return *this;
		  }
		  const_iterator operator-=(const int &n) {
			  //TODO
        pos_ -= n;
        return *this;
		  }
		  /**
		   * TODO iter++
		   */
		  const_iterator operator++(int) {
        const_iterator tmp(ptr_, pos_);
        pos_++;
        return tmp;
      }
		  /**
		   * TODO ++iter
		   */
		  const_iterator& operator++() {
        pos_++;
        return *this;
      }
		  /**
		   * TODO iter--
		   */
		  const_iterator operator--(int) {
        const_iterator tmp(ptr_, pos_);
        pos_--;
        return tmp;
      }
		  /**
		   * TODO --iter
		   */
		  const_iterator& operator--() {
        pos_--;
        return *this;
      }
			const T& operator*() const {
        return (*ptr_)[pos_];
      }
			const T* operator->() const noexcept {
        return &(*ptr_)[pos_];
      }
			bool operator==(const iterator &rhs) const {
        return ptr_ == rhs.ptr_ && pos_ == rhs.pos_;
      }
			bool operator==(const const_iterator &rhs) const {
        return ptr_ == rhs.ptr_ && pos_ == rhs.pos_;
      }
			bool operator!=(const iterator &rhs) const {
        return ptr_ != rhs.ptr_ || pos_ != rhs.pos_;
      }
			bool operator!=(const const_iterator &rhs) const {
        return ptr_ != rhs.ptr_ || pos_ != rhs.pos_;
      }
	};
  T **ptr_store_ = nullptr;
  int head_ = 0;
  int start_index = 0;
  int size_ = 0;
  size_t capacity_ = 0;
  inline size_t BlockIndex(const int &pos) const {
    return (head_ + (pos + start_index) / BlockSize) % capacity_;
  }
  inline size_t InnerIndex(const int &pos) const {
    return (pos + start_index + BlockSize) % BlockSize;
  }
  inline int BlockNum() const {
    return size_ == 0 ? 0 : ((size_ + start_index - 1) / BlockSize + 1);
  }
	/**
	 * TODO Constructors
	 */
	deque() = default;
	deque(const deque &other) {
    if(!other.empty()) {
      capacity_ = other.capacity_;
      head_ = other.head_;
      size_ = other.size_;
      start_index = other.start_index;
      ptr_store_ = new T *[capacity_];
      int block_num = BlockNum();
      for (int i = 0; i < block_num; i++) {
        ptr_store_[(head_ + i) % capacity_] = reinterpret_cast<T *>(::operator new(BlockSize * sizeof(T)));
      }
      for(int i = 0; i < size_; i++) {
        new (ptr_store_[BlockIndex(i)] + InnerIndex(i)) T(other.ptr_store_[BlockIndex(i)][InnerIndex(i)]);
      }
    }
  }
	/**
	 * TODO Deconstructor
	 */
	~deque() {
    clear();
  }
	/**
	 * TODO assignment operator
	 */
	deque &operator=(const deque &other) {
    if(&other != this) {
      deque tmp_deque(other);
      T **tmp_ptr = ptr_store_;
      ptr_store_ = tmp_deque.ptr_store_;
      tmp_deque.ptr_store_ = tmp_ptr;
      tmp_deque.head_ = head_;
      tmp_deque.size_ = size_;
      tmp_deque.start_index = start_index;
      tmp_deque.capacity_ = capacity_;
      head_ = other.head_;
      size_ = other.size_;
      start_index = other.start_index;
      capacity_ = other.capacity_;
    }
    return *this;
  }
	/**
	 * access specified element with bounds checking
	 * throw index_out_of_bound if out of bound.
	 */
	T & at(const size_t &pos) {
    //std::cerr << size() << std::endl;
    if(pos >= size_) {
      //std::cerr << "at " << pos << " " << head_ << " " << tail_ << " " << size() << std::endl;
      throw index_out_of_bound();
    }
    //std::cerr << BlockIndex(pos) << " " << head_ << " " << head_size_ << std::endl;
    return ptr_store_[BlockIndex(pos)][InnerIndex(pos)];
  }
	const T & at(const size_t &pos) const {
    if(pos >= size_) {
       //std::cerr << "at " << pos << " " << size() << std::endl;
      throw index_out_of_bound();
    }
    return ptr_store_[BlockIndex(pos)][InnerIndex(pos)];
  }
	T & operator[](const size_t &pos) {
    return at(pos);
  }
	const T & operator[](const size_t &pos) const {
    return at(pos);
  }
	/**
	 * access the first element
	 * throw container_is_empty when the container is empty.
	 */
	const T & front() const {
    if(empty()) {
      throw container_is_empty();
    }
    return ptr_store_[BlockIndex(0)][InnerIndex(0)];
  }
  T & front() {
    if(empty()) {
      throw container_is_empty();
    }
    return ptr_store_[BlockIndex(0)][InnerIndex(0)];
  }
	/**
	 * access the last element
	 * throw container_is_empty when the container is empty.
	 */
	const T & back() const {
    if(empty()) {
      throw container_is_empty();
    }
    return ptr_store_[BlockIndex(size_ - 1)][InnerIndex(size_ - 1)];
  }
  T & back() {
    if(empty()) {
      throw container_is_empty();
    }
    return ptr_store_[BlockIndex(size_ - 1)][InnerIndex(size_ - 1)];
  }
	/**
	 * returns an iterator to the beginning.
	 */
	iterator begin() {
    return iterator(this, 0);
  }
	const_iterator cbegin() const {
    return const_iterator(this, 0);
  }
	/**
	 * returns an iterator to the end.
	 */
	iterator end() {
    return iterator(this, size_);
  }
	const_iterator cend() const {
    return const_iterator(this, size_);
  }
	/**
	 * checks whether the container is empty.
	 */
	bool empty() const {
    return size_ == 0;
  }
	/**
	 * returns the number of elements
	 */
	size_t size() const {
    return size_;
  }
	/**
	 * clears the contents
	 */
	void clear() {
    if(ptr_store_ != nullptr) {
      for(int i = 0; i < size_; i++) {
        ptr_store_[BlockIndex(i)][InnerIndex(i)].~T();
      }
      int block_num = BlockNum();
      for(int i = 0; i < block_num; i++) {
        ::operator delete(ptr_store_[(head_ + i) % capacity_]);
      }
    }
    delete [] ptr_store_;
    ptr_store_ = nullptr;
    head_ = 0;
    start_index =0;
    size_ = 0;
    capacity_ = 0;
  }
  void init() {
    if(ptr_store_ == nullptr) {
      ptr_store_ = new T*[4];
      capacity_ = 4;
      head_ = 0;
      start_index = 0;
      size_ = 0;
    }
  }
  void reserve(size_t n) {
    if(capacity_ >= n) {
      return ;
    }
    init();
    T** new_ptr_store = new T *[n];
    int block_num = BlockNum();
    for(int i = 0; i < block_num; i++) {
      new_ptr_store[i] = ptr_store_[(i + head_) % capacity_];
    }
    delete [] ptr_store_;
    ptr_store_ = new_ptr_store;
    head_ = 0;
    capacity_ = n;
  }
	/**
	 * inserts elements at the specified locat on in the container.
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value
	 *     throw if the iterator is invalid or it point to a wrong place.
	 */
	iterator insert(iterator pos, const T &value) {
    init();
    //std::cerr << "finish init" << std::endl;
    size_t index = pos - begin();
    //std::cerr << index << " " << size() << " " << tail_size_ << " " << head_ << " " << tail_ << " " << capacity_ << std::endl;
    if(index > size_) {
      //std::cerr << "insert " << index << " " << size() << std::endl;
      throw index_out_of_bound();
    }
    if(index == 0) {
      push_front(value);
      return begin();
    } 
    if(index == size_) {
      push_back(value);
      return end() - 1;
    }
    if(InnerIndex(size_) == 0 || size_ == 0) {
      if(BlockNum() == capacity_) {
        reserve(2 * capacity_);
      }
      ptr_store_[BlockIndex(size_)] = reinterpret_cast<T *>(::operator new(BlockSize * sizeof(T)));
    }
    new (ptr_store_[BlockIndex(size_)] + InnerIndex(size_)) T(std::move(ptr_store_[BlockIndex(size_ - 1)][InnerIndex(size_ - 1)]));
    for(size_t i = size_ - 1; i > index; i--) {
      ptr_store_[BlockIndex(i)][InnerIndex(i)] = std::move(ptr_store_[BlockIndex(i - 1)][InnerIndex(i - 1)]);
    }
    size_++;
    ptr_store_[BlockIndex(index)][InnerIndex(index)] = value;
    return iterator(this, index);
  }
	/**
	 * removes specified element at pos.
	 * removes the element at pos.
	 * returns an iterator pointing to the following element, if pos pointing to the last element, end() will be returned.
	 * throw if the container is empty, the iterator is invalid or it points to a wrong place.
	 */
	iterator erase(iterator pos) {
    if(empty()) {
      throw container_is_empty();
    }
    size_t index = pos - begin();
    //std::cerr << index << " " << size() << std::endl;
    if(index >= size_) {
       //std::cerr << "erase " << index << " " << size() << std::endl;
      throw index_out_of_bound();
    }
    if(index == 0) {
      pop_front();
      return begin();
    }
    if(index == size_ - 1) {
      pop_back();
      return end();
    }
    for(size_t i = index; i < size_ - 1; i++) {
      ptr_store_[BlockIndex(i)][InnerIndex(i)] = std::move(ptr_store_[BlockIndex(i + 1)][InnerIndex(i + 1)]);
    }
    size_--;
    ptr_store_[BlockIndex(size_)][InnerIndex(size_)].~T();
    if(InnerIndex(size_) == 0 || size_ == 0) { // only one block, the index may not be 0
      ::operator delete(ptr_store_[BlockIndex(size_)]);
    }
    //std::cerr << size() << std::endl;
    return iterator(this, index);
  }
	/**
	 * adds an element to the end
	 */
	void push_back(const T &value) {
    init();
    if(InnerIndex(size_) == 0 || size_ == 0) {
      //std::cerr << tail_ << " " << capacity_ << " " << head_ << std::endl;
      if(BlockNum() == capacity_) {
        reserve(2 * capacity_);
      }
      ptr_store_[BlockIndex(size_)] = reinterpret_cast<T *>(::operator new(BlockSize * sizeof(T)));
    }
    //std::cerr << head_ << " " << tail_ << " " << tail_size_ << " " << capacity_ << std::endl;
    //std::cerr << BlockIndex(size_) << " " << InnerIndex(size_) << BlockNum() << std::endl;
    new (ptr_store_[BlockIndex(size_)] + InnerIndex(size_)) T(value);
    size_++;
  }
  template <typename... Args>
  void emplace_back(Args&&... args) {
    init();
    if(InnerIndex(size_) == 0 || size_ == 0) {
      //std::cerr << tail_ << " " << capacity_ << " " << head_ << std::endl;
      if(BlockNum() == capacity_) {
        reserve(2 * capacity_);
      }
      ptr_store_[BlockIndex(size_)] = reinterpret_cast<T *>(::operator new(BlockSize * sizeof(T)));
    }
    //std::cerr << head_ << " " << tail_ << " " << tail_size_ << " " << capacity_ << std::endl;
    //std::cerr << BlockIndex(size_) << " " << InnerIndex(size_) << BlockNum() << std::endl;
    new (ptr_store_[BlockIndex(size_)] + InnerIndex(size_)) T(std::forward<Args>(args)...);
    size_++;
  }
	/**
	 * removes the last element
	 *     throw when the container is empty.
	 */
	void pop_back() {
    if(empty()) {
      throw container_is_empty();
    }
    //std::cerr << size_ << " " << BlockIndex(size_ - 1) << " " << InnerIndex(size_ - 1) << std::endl;
    ptr_store_[BlockIndex(size_ - 1)][InnerIndex(size_ - 1)].~T();
    size_--;
    if(InnerIndex(size_) == 0 || size_ == 0) {
      ::operator delete(ptr_store_[BlockIndex(size_)]);
    }
  }
	/**
	 * inserts an element to the beginning.
	 */
	void push_front(const T &value) {
    init();
    //std::cerr << "init" << std::endl;
    if(start_index == 0 || size_ == 0) {
      if(BlockNum() == capacity_) {
        reserve(2 * capacity_);
      }
      head_ = (head_ - 1 + capacity_) % capacity_;
      ptr_store_[head_] = reinterpret_cast<T *>(::operator new(BlockSize * sizeof(T)));
      start_index = BlockSize;
    }
    start_index--;
    new (ptr_store_[head_] + start_index) T(value);
    size_++;
    //std::cerr << head_ << " " << head_size_ << std::endl;
  }
	/**
	 * removes the first element.
	 *     throw when the container is empty.
	 */
	void pop_front() {
    if(empty()) {
      throw container_is_empty();
    }
    //std::cerr << head_size_ << " " << head_ << " " << BlockIndex(0) << std::endl;
    ptr_store_[BlockIndex(0)][InnerIndex(0)].~T();
    start_index++;
    size_--;
    if(start_index == BlockSize || size_ == 0) {
      ::operator delete(ptr_store_[head_]);
      head_ = (head_ + 1) % capacity_;
      start_index = 0;
    }
  }
};

}

#endif
