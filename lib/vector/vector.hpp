#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP


#include "exceptions.hpp"


namespace sjtu
{
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */

/*
template <typename T> struct RemoveReference { using Type = T; };
template <typename T> struct RemoveReference<T&> { using Type = T; };
template <typename T> struct RemoveReference<T&&> { using Type = T; };

template <typename T>
typename RemoveReference<T>::Type&& move(T&& arg) {
    return static_cast<typename RemoveReference<T>::Type&&>(arg);
}
*/
template<typename T>
class vector
{
public:
	/**
	 * TODO
	 * a type for actions of the elements of a vector, and you should write
	 *   a class named const_iterator with same interfaces.
	 */
	/**
	 * you can see RandomAccessIterator at CppReference for help.
	 */
  T *data_ = nullptr;
  int size_ = 0;
  int capacity_ = 0;
	class const_iterator;

	class iterator {
	// The following code is written for the C++ type_traits library.
	// Type traits is a C++ feature for describing certain properties of a type.
	// For instance, for an iterator, iterator::value_type is the type that the
	// iterator points to.
	// STL algorithms and containers may use these type_traits (e.g. the following
	// typedef) to work properly. In particular, without the following code,
	// @code{std::sort(iter, iter1);} would not compile.
	// See these websites for more information:
	// https://en.cppreference.com/w/cpp/header/type_traits
	// About value_type: https://blog.csdn.net/u014299153/article/details/72419713
	// About iterator_category: https://en.cppreference.com/w/cpp/iterator
   friend const_iterator;
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;

	private:
		/**
		 * TODO add data members
		 *   just add whatever you want.
		 */
    vector *parent_ptr_;
    T *ptr_;
	public:
    iterator(vector *parent_ptr, T *ptr):parent_ptr_(parent_ptr), ptr_(ptr) {}
		/**
		 * return a new iterator which pointer n-next elements
		 * as well as operator-
		 */
		iterator operator+(const int &n) const
		{
      return iterator(parent_ptr_, ptr_ + n);
			//TODO
		}
		iterator operator-(const int &n) const
		{
      return iterator(parent_ptr_, ptr_ - n);
			//TODO
		}
		// return the distance between two iterators,
		// if these two iterators point to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const
		{
      if(parent_ptr_ != rhs.parent_ptr_) {
        throw invalid_iterator();
      }
      return ptr_ - rhs.ptr_;
			//TODO
		}
		iterator& operator+=(const int &n)
		{
      ptr_ += n;
      return *this;
			//TODO
		}
		iterator& operator-=(const int &n)
		{
			//TODO
      ptr_ -= n;
      return *this;
		}
		/**
		 * TODO iter++
		 */
		iterator operator++(int) {
      iterator tmp(parent_ptr_, ptr_);
      ptr_++;
      return tmp;
    }
		/**
		 * TODO ++iter
		 */
		iterator& operator++() {
      ptr_ += 1;
      return *this;
    }
		/**
		 * TODO iter--
		 */
		iterator operator--(int) {
      iterator tmp(parent_ptr_, ptr_);
      ptr_--;
      return tmp;
    }
		/**
		 * TODO --iter
		 */
		iterator& operator--() {
      ptr_ -= 1;
      return *this;
    }
		/**
		 * TODO *it
		 */
		T& operator*() const{
      return *ptr_;
    }
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory address).
		 */
		bool operator==(const iterator &rhs) const {
      return ptr_ == rhs.ptr_;
    }
		bool operator==(const const_iterator &rhs) const {
      return ptr_ == rhs.ptr_;
    }
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
      return ptr_ != rhs.ptr_;
    }
		bool operator!=(const const_iterator &rhs) const {
      return ptr_ != rhs.ptr_;
    }
	};
	/**
	 * TODO
	 * has same function as iterator, just for a const object.
	 */
	class const_iterator {
    friend iterator;
	 public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T*;
		using reference = T&;
		using iterator_category = std::output_iterator_tag;
    const_iterator(const vector *parent_ptr, const T *ptr):parent_ptr_(parent_ptr), ptr_(ptr) {}
    const_iterator(const iterator &iter):parent_ptr_(iter.patent_ptr_), ptr_(iter.ptr_) {}
    const_iterator operator+(const int &n) const
		{
      return const_iterator(parent_ptr_, ptr_ + n);
		}
		const_iterator operator-(const int &n) const
		{
      return const_iterator(parent_ptr_, ptr_ - n);
			//TODO
		}
		// return the distance between two iterators,
		// if these two iterators point to different vectors, throw invaild_iterator.
		int operator-(const iterator &rhs) const
		{
      if(parent_ptr_ != rhs.parent_ptr_) {
        throw invalid_iterator();
      }
      return ptr_ - rhs.ptr_;
			//TODO
		}
		const_iterator& operator+=(const int &n)
		{
      ptr_ += n;
      return *this;
			//TODO
		}
		const_iterator& operator-=(const int &n)
		{
			//TODO
      ptr_ -= n;
      return *this;
		}
		/**
		 * TODO iter++
		 */
		const_iterator operator++(int) {
      const_iterator tmp(parent_ptr_, ptr_);
      ptr_++;
      return tmp;
    }
		/**
		 * TODO ++iter
		 */
		const_iterator& operator++() {
      ptr_ += 1;
      return *this;
    }
		/**
		 * TODO iter--
		 */
		const_iterator operator--(int) {
      const_iterator tmp(parent_ptr_, ptr_);
      ptr_--;
      return tmp;
    }
		/**
		 * TODO --iter
		 */
		const_iterator& operator--() {
      ptr_ -= 1;
      return *this;
    }
		/**
		 * TODO *it
		 */
		const T& operator*() const{
      return *ptr_;
    }
		/**
		 * a operator to check whether two iterators are same (pointing to the same memory address).
		 */
		bool operator==(const iterator &rhs) const {
      return ptr_ == rhs.ptr_;
    }
		bool operator==(const const_iterator &rhs) const {
      return ptr_ == rhs.ptr_;
    }
		/**
		 * some other operator for iterator.
		 */
		bool operator!=(const iterator &rhs) const {
      return ptr_ != rhs.ptr_;
    }
		bool operator!=(const const_iterator &rhs) const {
      return ptr_ != rhs.ptr_;
    }

	private:
		/*TODO*/
    const vector *parent_ptr_;
    const T *ptr_;
	};
	/**
	 * TODO Constructs
	 * At least two: default constructor, copy constructor
	 */
	vector() = default;
  vector(int n) {
    data_ = static_cast<T *>(::operator new(n * sizeof(T)));
    capacity_ = n;
    size_ = 0;
  }
  vector(int n, const T &value) {
    data_ = static_cast<T *>(::operator new(n * sizeof(T)));
    capacity_ = n;
    size_ = n;
    for(int i = 0; i < n; i++) {
      new (data_ + i) T(value);
    }
  }
	vector(const vector &other) {
    capacity_ = other.capacity_;
    size_ = other.size_;
    data_ = static_cast<T *>(::operator new(capacity_ * sizeof(T)));
    if(std::is_trivially_copyable_v<T>) {
      std::memcpy(data_, other.data_, other.size_ * sizeof(T));
    } else {
      for(int i = 0; i < size_; i++) {
        new (data_ + i) T(other.data_[i]); 
      }
    }
  }
  vector(vector &&other) {
    capacity_ = other.capacity_;
    size_ = other.size_;
    data_ = other.data_;
    other.capacity_ = 0;
    other.size_ = 0;
    other.data_ = nullptr;
  }
	/**
	 * TODO Destructor
	 */
	~vector() {
    clear();
  }
	/**
	 * TODO Assignment operator
	 */
	vector &operator=(const vector &other) {
    if(&other != this) {
      if(capacity_ > other.size_) {
        if constexpr (std::is_trivially_copyable_v<T>) {
          std::memcpy(data_, other.data_, other.size_ * sizeof(T));
        } else {
          for(int i = 0; i < size_; i++) {
            data_[i].~T();
          }
          for(int i = 0; i < other.size_; i++) {
            new (data_ + i) T(other.data_[i]);
          }
        }
        size_ = other.size_;
      } else {
        vector tmp(other);
        int temp = tmp.capacity_;
        tmp.capacity_ = capacity_;
        capacity_ = temp;
        temp = tmp.size_;
        tmp.size_ = size_;
        size_ = temp;
        T *temp_ptr = tmp.data_;
        tmp.data_ = data_;
        data_ = temp_ptr;
      }
    }
    return *this;
  }
  vector &operator=(vector &&other) {
    if(&other != this) {
      clear();
      size_ = other.size_;
      capacity_ = other.capacity_;
      data_ = other.data_;
      other.data_ = nullptr;
    }
    return *this;
  }
  T *data() {
    return data_;
  }
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 */
	T & at(const size_t &pos) {
    if(pos >= size_) {
      throw index_out_of_bound();
    }
    return data_[pos];
  }
	const T & at(const size_t &pos) const {
    if(pos >= size_) {
      throw index_out_of_bound();
    }
    return data_[pos];
  }
	/**
	 * assigns specified element with bounds checking
	 * throw index_out_of_bound if pos is not in [0, size)
	 * !!! Pay attentions
	 *   In STL this operator does not check the boundary but I want you to do.
	 */
	T & operator[](const size_t &pos) {
    return at(pos);
  }
	const T & operator[](const size_t &pos) const {
    return at(pos);
  }
	/**
	 * access the first element.
	 * throw container_is_empty if size == 0
	 */
	const T & front() const {
    if(size_ == 0) {
      throw container_is_empty();
    }  
    return data_[0];
  }
	/**
	 * access the last element.
	 * throw container_is_empty if size == 0
	 */
	const T & back() const {
    if(size_ == 0) {
      throw container_is_empty();
    }  
    return data_[size_ - 1];
  }
	/**
	 * returns an iterator to the beginning.
	 */
	iterator begin() {
    return iterator{this, data_};
  }
	const_iterator begin() const {
    return const_iterator{this, data_};
  }
	const_iterator cbegin() const {
    return begin();
  }
	/**
	 * returns an iterator to the end.
	 */
	iterator end() {
    return iterator{this, data_ + size_};
  }
	const_iterator end() const {
    return const_iterator{this, data_ + size_};
  }
	const_iterator cend() const {
    return end();
  }
	/**
	 * checks whether the container is empty
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
    if(data_) {
      for(int i = 0; i < size_; i++) {
        data_[i].~T();
      }
      ::operator delete(data_);
      data_ = nullptr;
    }
    size_ = 0;
    capacity_ = 0;
  }
  void reserve(int n) {
    //printf("%d\n", n);
    if(n <= capacity_) {
      return ;
    }
    capacity_ = n;
    T *new_data = static_cast<T *>(::operator new(capacity_ * sizeof(T)));
    if(data_) {
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(new_data, data_, size_ * sizeof(T));
      } else {
        for(int i = 0; i < size_; i++) {
          new (new_data + i) T(std::move(data_[i]));
          data_[i].~T();
        }
      }
      ::operator delete(data_);
    }
    data_ = new_data;
  }
	/**
	 * inserts value before pos
	 * returns an iterator pointing to the inserted value.
	 */
	iterator insert(iterator pos, const T &value) {
    return insert(pos - begin(), value);
  }
	/**
	 * inserts value at index ind.
	 * after inserting, this->at(ind) == value
	 * returns an iterator pointing to the inserted value.
	 * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will increase 1.)
	 */
	iterator insert(const size_t &ind, const T &value) {
    if(ind > size_) {
      throw index_out_of_bound();
    }
    if(size_ == capacity_) {
      reserve(2 * capacity_ + 1);
    }
    if(size_ == ind) {
      new (data_ + size_) T(value);
    } else {
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memmove(data_ + ind + 1, data_ + ind, (size_ - ind) * sizeof(T));
      } else {
        new (data_ + size_) T(data_[size_ - 1]);
        for(int i = size_ - 1; i > ind; i--) {
          data_[i] = std::move(data_[i - 1]);
        }
      }
      data_[ind] = value;
    }
    size_++;
    return iterator(this, data_ + ind);
  }
	/**
	 * removes the element at pos.
	 * return an iterator pointing to the following element.
	 * If the iterator pos refers the last element, the end() iterator is returned.
	 */
	iterator erase(iterator pos) {
    return erase(pos - begin());
  }
	/**
	 * removes the element with index ind.
	 * return an iterator pointing to the following element.
	 * throw index_out_of_bound if ind >= size
	 */
	iterator erase(const size_t &ind) {
    if (ind >= size_) {
      throw index_out_of_bound();
    } 
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memmove(data_ + ind, data_ + ind + 1, (size_ - ind - 1) * sizeof(T));
    } else {
      for (int i = ind; i < size_ - 1; i++) {
        data_[i] = std::move(data_[i + 1]);
      }
    }
    data_[size_ - 1].~T();
    size_--;
    return iterator(this, data_ + ind);
  }
	/**
	 * adds an element to the end.
	 */
	void push_back(const T &value) {
    if(size_ == capacity_) {
      reserve(2 * capacity_ + 1);
    }
    new (data_ + size_) T(value);
    size_++;
  }
	/**
	 * remove the last element from the end.
	 * throw container_is_empty if size() == 0
	 */
	void pop_back() {
    if(empty()) {
      throw container_is_empty();
    }
    data_[size_ - 1].~T();
    size_--;
  }
};


}

#endif
