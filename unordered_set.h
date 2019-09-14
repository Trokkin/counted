#pragma once
#include <functional>
#include <memory>
#include "set.h"
#include "vector.h"

size_t DEFAULT_POW = 4;
float THRESHOLD = 0.75;

template <typename T, class Hash = std::hash<T>>
class unordered_set {
  vector<set<const T>> storage;
  size_t size;
  size_t maxsize;

  template <typename C>
  struct iterator_t {
    using v_it_t = typename vector<set<const C>>::const_iterator;
    using s_it_t = typename set<const C>::const_iterator;
    v_it_t v_it;
    v_it_t end;
    s_it_t s_it;

    using difference_type = std::ptrdiff_t;
    using value_type = C;
    using pointer = C*;
    using reference = C&;
    using iterator_category = std::forward_iterator_tag;

    iterator_t() : v_it(nullptr), end(nullptr), s_it(){};
    explicit iterator_t(v_it_t v_it_, v_it_t end_)
        : v_it(v_it_), end(end_), s_it() {
      if (v_it != end) {
        s_it = v_it->begin();
        check();
      }
    }
    explicit iterator_t(v_it_t v_it, v_it_t end, s_it_t s_it)
        : v_it(v_it), end(end), s_it(s_it) {}

    C& operator*() const { return *s_it; }
    C* operator->() const { return &(*s_it); }

    void check() {
      while (s_it == v_it->end())
        if (++v_it == end) {
          s_it = s_it_t();
          break;
        } else {
          s_it = v_it->begin();
        }
    }

    iterator_t& operator++() {
      if (v_it == end) return *this;
      s_it++;
      check();
      return *this;
    }
    const iterator_t operator++(int) {
      iterator_t t(*this);
      ++(*this);
      return t;
    }

    friend bool operator==(iterator_t const& a, iterator_t const& b) {
      return a.s_it == b.s_it && a.v_it == b.v_it;
    }
    friend bool operator!=(iterator_t const& a, iterator_t const& b) {
      return a.s_it != b.s_it || a.v_it != b.v_it;
    }
  };

  void resize_vector(size_t capacity) {
    unordered_set<T> new_set(capacity);
    for (const T& t : *this) new_set.insert(t);
    swap(*this, new_set);
  }
  void check_size() {
    if (size >= maxsize) {
      maxsize *= THRESHOLD;
      resize_vector(storage.size() << 1);
    }
  }

 public:
  using value_type = T;
  using const_reference = T const&;
  using reference = T&;
  using const_pointer = T const*;
  using pointer = T*;

  using const_iterator = iterator_t<const T>;
  using iterator = const_iterator;

  unordered_set()
      : storage(), size(0), maxsize((1 << DEFAULT_POW) * THRESHOLD) {
    storage.resize(1 << DEFAULT_POW);
  }
  unordered_set(size_t hash_pow)
      : storage(), size(0), maxsize((1 << hash_pow) * THRESHOLD) {
    storage.resize(1 << hash_pow);
  }
  unordered_set(unordered_set const& other)
      : storage(other.storage), size(other.size), maxsize(other.maxsize) {}
  unordered_set& operator=(unordered_set other) {
    swap(*this, other);
    return *this;
  }
  ~unordered_set() { clear(); }

  bool empty() const noexcept { return size == 0; }
  void clear() noexcept {
    storage.clear();
    storage.resize(1 << DEFAULT_POW);
    size = 0;
    maxsize = (1 << DEFAULT_POW) * THRESHOLD;
  }

  const_iterator begin() const noexcept {
    return const_iterator(storage.begin(), storage.end());
  }
  const_iterator end() const noexcept {
    return const_iterator(storage.end(), storage.end());
  }
  iterator begin() noexcept { return iterator(storage.begin(), storage.end()); }
  iterator end() noexcept { return iterator(storage.end(), storage.end()); }

  iterator insert(const_iterator pos, const_reference v) {
    check_size();
    size++;
    auto* e = &storage[Hash{}(v) % storage.size()];
    typename iterator::s_it_t i;
    if (pos.v_it == e)
      i = e->insert(pos.s_it, v);
    else
      i = e->find(v);
    if (i != e->end()) {
      return iterator(e, storage.end(), i);
    }
    return iterator(e, storage.end(), e->insert(v));
  }

  iterator erase(const_iterator pos) {
    assert(storage.end() == pos.end);
    size--;
    auto i = iterator(
        pos.v_it, pos.end,
        (storage.begin() + (pos.v_it - storage.begin()))->erase(pos.s_it));
    i.check();
    return i;
  }

  std::pair<iterator, bool> insert(const_reference v) {
    check_size();
    size++;
    auto s = Hash{}(v) % storage.size();
    auto* e = &storage[s];
    auto i = e->find(v);
    if (i != e->end()) {
      return {iterator(storage.begin() + s, storage.end(), i), false};
    }
    return {iterator(storage.begin() + s, storage.end(), e->insert(v).first),
            true};
  }

  iterator find(const_reference v) const {
    if (empty()) return end();
    auto s = Hash{}(v) % storage.size();
    auto* e = &storage[Hash{}(v) % storage.size()];
    auto i = e->find(v);
    return (i != e->end() ? iterator(storage.begin() + s, storage.end(), i)
                          : end());
  }

  template <typename V>
  friend void swap(unordered_set<V>&, unordered_set<V>&) noexcept;
};

template <typename V>
void swap(unordered_set<V>& a, unordered_set<V>& b) noexcept {
  swap(a.storage, b.storage);
  std::swap(a.size, b.size);
  std::swap(a.maxsize, b.maxsize);
}
