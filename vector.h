#pragma once

#include <algorithm>
#include <memory>
#include <variant>

/**
 * sizeof(vector<T>) <= max(2*sizeof(void*), sizeof(void*) + sizeof(T))
 * Максимум одна аллокация вне вектора (внутри функций может быть больше)
 * Релизовать все функции который указаны в слаке
 * Гарантии для всех strong, за исключением константных (они noexcept)
 * insert, erase - base гарантии (если вставка/удаление в/до конца, то strong)
 * SmallObject (на 1 объект)
 * CopyOnWrite
 * T не имеет дефолтного конструктора
 * Деструктор Т не кидает исключений, остальное не определено
 */

template <typename T>
class vector {
  const size_t DEFAULT_VEC_SIZE = 4;

  struct shared_array {
    size_t capacity;
    size_t size;
    size_t owners;
    T data[];

    bool full() { return size == capacity; }
    T* end() { return data + size; }
    void destroy() {
      std::destroy_n(data, size);
      operator delete(this);
    }
  };

  /** Invariant:
   * data_.index() == 0 -> u_ == std::monostate
   * data_.index() == 1 -> u_ == T
   * data_.index() == 2 -> u_ == shared_array*
   */
  std::variant<std::monostate, T, shared_array*> data_;

  shared_array* new_shared(size_t capacity) {
    auto mem = operator new(sizeof(shared_array) + capacity * sizeof(T));
    try {
      new (mem) shared_array{capacity, 0, 1};
    } catch (...) {
      operator delete(mem);
      throw;
    }
    shared_array* n = reinterpret_cast<shared_array*>(mem);
    return n;
  }
  shared_array* resize_vector(size_t capacity) {
    shared_array* n = new_shared(capacity);
    if (data_.index() == 1) {
      n->size = 1;
      try {
        new (n->data) T(std::get<1>(data_));
      } catch (...) {
        operator delete(n);
        throw;
      }
    } else if (data_.index() == 2) {
      n->size = std::min(n->capacity, std::get<2>(data_)->size);
      try {
        std::uninitialized_copy_n(std::get<2>(data_)->data, n->size, n->data);
      } catch (...) {
        operator delete(n);
        throw;
      }
    }
    return n;
  }
  void set_data(shared_array* n) {
    if (data_.index() == 2 && n != std::get<2>(data_)) {
      std::get<2>(data_)->owners--;
      if (std::get<2>(data_)->owners == 0) std::get<2>(data_)->destroy();
    }
    data_ = n;
  }
  void set_data(T const& v) {
    if (data_.index() == 2) {
      std::get<2>(data_)->owners--;
      if (std::get<2>(data_)->owners == 0) std::get<2>(data_)->destroy();
    }
    data_ = v;
  }

 public:
  typedef T value_type;
  typedef T const& const_reference;
  typedef T& reference;
  typedef T const* const_pointer;
  typedef T* pointer;

  typedef T const* const_iterator;
  typedef T* iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;

  vector() noexcept : data_(std::monostate()) {}
  ~vector() { clear(); }

  bool empty() const noexcept { return data_.index() == 0; };
  size_t size() const noexcept {
    return data_.index() == 2 ? std::get<2>(data_)->size : data_.index();
  }
  size_t capacity() const noexcept {
    return data_.index() == 2 ? std::get<2>(data_)->capacity : 1;
  }
  void clear() {
    if (data_.index() == 2) {
      std::get<2>(data_)->owners--;
      if (std::get<2>(data_)->owners == 0) std::get<2>(data_)->destroy();
    }
    data_ = std::monostate();
  }

  const_pointer data() const noexcept {
    if (data_.index() == 2) {
      return std::get<2>(data_)->data;
    } else if (data_.index() == 1) {
      return &std::get<1>(data_);
    }
    return nullptr;
  }

  pointer data() {
    if (data_.index() == 2) {
      if (std::get<2>(data_)->owners > 1)
        set_data(resize_vector(std::get<2>(data_)->size));
      return std::get<2>(data_)->data;
    } else if (data_.index() == 1) {
      return &std::get<1>(data_);
    }
    return nullptr;
  }

  const_reference operator[](size_t index) const noexcept {
    return data()[index];
  }
  reference operator[](size_t index) { return data()[index]; }

  const_reference front() const noexcept { return operator[](0); }
  const_reference back() const noexcept { return operator[](size() - 1); }
  reference front() { return operator[](0); }
  reference back() { return operator[](size() - 1); }

  const_iterator begin() const noexcept { return data(); }
  const_iterator end() const noexcept { return data() + size(); }
  iterator begin() noexcept { return data(); }
  iterator end() noexcept { return data() + size(); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  vector(vector const& other) noexcept : data_(std::monostate()) {
    data_ = other.data_;
    if (data_.index() == 2) {
      std::get<2>(data_)->owners++;
    }
  }
  vector& operator=(vector const& other) {
    vector temp(other);
    swap(*this, temp);
    return *this;
  }

  template <typename InputIterator>
  vector(InputIterator first, InputIterator last) : data_(std::monostate()) {
    size_t size = std::distance(first, last);
    if (size > 1) {
      shared_array* t = new_shared(size);
      try {
        std::uninitialized_copy(first, last, t->data);
        t->size = size;
      } catch (...) {
        operator delete(t);
      }
      data_ = t;
    } else if (size == 1) {
      data_ = *first;
    } else {
      data_ = std::monostate();
    }
  }
  template <typename InputIterator>
  void assign(InputIterator first, InputIterator last) {
    vector temp(first, last);
    std::swap(*this, temp);
  }

  void push_back(const_reference v) {
    if (data_.index() == 0) {
      data_ = v;
    } else {
      shared_array* na;
      if (data_.index() == 2) {
        na = resize_vector(std::get<2>(data_)->capacity *
                           (std::get<2>(data_)->full() ? 2 : 1));
      } else {
        na = resize_vector(DEFAULT_VEC_SIZE);
      }
      try {
        new (na->end()) T(v);
      } catch (...) {
        operator delete(na);
        throw;
      }
      na->size++;
      set_data(na);
    }
  }

  void pop_back() {
    if (data_.index() == 2) {
      if (std::get<2>(data_)->size == 2) {
        T v = std::get<2>(data_)->data[0];
        set_data(v);
      } else {
        shared_array* t = resize_vector(std::get<2>(data_)->capacity);
        t->data[--t->size].~T();
        set_data(t);
      }
    } else if (data_.index() == 1) {
      data_ = std::monostate();
    }
  }

  void shrink_to_fit() {
    if (data_.index() == 2 && !std::get<2>(data_)->full()) {
      set_data(resize_vector(std::get<2>(data_)->size));
    }
  }

  void resize(size_t new_size) {
    if (size() == new_size) return;
    shared_array* t;
    if (new_size > 1) {
      t = resize_vector(new_size);
      try {
        std::uninitialized_value_construct_n(t->end(), new_size - t->size);
      } catch (...) {
        operator delete(t);
        throw;
      }
      t->size = new_size;
      set_data(t);
    } else if (new_size == 1) {
      if (data_.index() == 2) {
        T v = std::get<2>(data_)->data[0];
        set_data(v);
      } else {
        data_ = T();
      }
    } else {
      clear();
    }
  }
  void resize(size_t new_size, const_reference default_value) {
    if (size() == new_size) return;
    shared_array* t;
    if (new_size > 1) {
      t = resize_vector(new_size);
      try {
        std::uninitialized_fill_n(t->end(), new_size - t->size, default_value);
      } catch (...) {
        operator delete(t);
        throw;
      }
      t->size = new_size;
      set_data(t);
    } else if (new_size == 1) {
      if (data_.index() == 2) {
        T v = std::get<2>(data_)->data[0];
        set_data(v);
      } else {
        data_ = T(default_value);
      }
    } else {
      clear();
    }
  }
  void reserve(size_t new_capacity) {
    if (size() >= new_capacity || new_capacity < 2) return;
    set_data(resize_vector(new_capacity));
  }

  iterator insert(const_iterator pos, const_reference val) {
    size_t i = pos ? pos - begin() : 0;
    push_back(val);
    if (data_.index() == 2) {
      std::rotate(begin() + i, end() - 1, end());
    }
    return begin() + i;
  }
  iterator erase(const_iterator pos) { return erase(pos, pos + 1); }
  iterator erase(const_iterator first, const_iterator last) {
    if (first >= last) return begin() + (first - begin());
    auto d = std::distance(first, last);
    size_t i = first - begin();
    size_t j = last - begin();
    if (data_.index() == 2) {
      if (i == 0 && j == std::get<2>(data_)->size) {
        clear();
      } else if ((int)std::get<2>(data_)->size == d + 1) {
        T v = std::get<2>(data_)->data[0];
        set_data(v);
      } else {
        shared_array* t = resize_vector(std::get<2>(data_)->capacity);
        try {
          std::move(t->data + j, t->end(), t->data + i);
          std::destroy(t->end() - d, t->end());
        } catch (...) {
          operator delete(t);
          throw;
        }
        t->size -= d;
        set_data(t);
      }
    } else if (data_.index() == 1) {
      data_ = std::monostate();
    }
    return begin() + i;
  }

  template <typename V>
  friend void swap(vector<V>&, vector<V>&) noexcept;
};

template <typename V>
void swap(vector<V>& a, vector<V>& b) noexcept {
  std::swap(a.data_, b.data_);
}

template <typename T>
bool operator==(vector<T> const& a, vector<T> const& b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T>
bool operator!=(vector<T> const& a, vector<T> const& b) {
  return !(a == b);
}

template <typename T>
bool operator<(vector<T> const& a, vector<T> const& b) {
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

template <typename T>
bool operator<=(vector<T> const& a, vector<T> const& b) {
  return !(b < a);
}

template <typename T>
bool operator>(vector<T> const& a, vector<T> const& b) {
  return b < a;
}

template <typename T>
bool operator>=(vector<T> const& a, vector<T> const& b) {
  return !(a < b);
}
