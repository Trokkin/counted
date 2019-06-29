#pragma once

template <typename T>
class set {
  struct node {
    node* left;
    node* right;
    node* parent;
    node() : left(nullptr), right(nullptr), parent(nullptr) {}
    node(node* parent) : left(nullptr), right(nullptr), parent(parent) {}
    node(node* left, node* right, node* parent)
        : left(left), right(right), parent(parent) {}
    virtual ~node() = default;

    void destroy() {
      if (left) left->destroy();
      if (right) right->destroy();
      delete this;
    }
  };

  struct node_v : public node {
    T value;
    node_v(T const& v) : node(), value(v) {}
    node_v(T const& v, node* parent) : node(parent), value(v) {}
    node_v(T const& v, node* left, node* right, node* parent)
        : node(left, right, parent), value(v) {}
  };
  node* copy_tree(node* t, node* parent) {
    node* n = new node_v(static_cast<node_v*>(t)->value, parent);
    if (t->left) n->left = copy_tree(t->left, n);
    if (t->right) n->right = copy_tree(t->right, n);
    return n;
  }
  node dummy;

  template <typename C>
  struct iterator_t {
    node* ref;

    using difference_type = std::ptrdiff_t;
    using value_type = C;
    using pointer = C*;
    using reference = C&;
    using iterator_category = std::bidirectional_iterator_tag;

    iterator_t() : ref(nullptr){};
    explicit iterator_t(node* v) : ref(v) {}

    C& operator*() const { return static_cast<node_v*>(ref)->value; }
    C* operator->() const { return &(static_cast<node_v*>(ref)->value); }
    iterator_t& operator++() {
      if (ref == nullptr) return *this;
      if (ref->right != nullptr) {
        ref = ref->right;
        while (ref->left != nullptr) ref = ref->left;
        return *this;
      }
      while (ref->parent != nullptr && ref->parent->right == ref)
        ref = ref->parent;
      if (ref->parent != nullptr) ref = ref->parent;
      return *this;
    }
    iterator_t& operator--() {
      if (ref == nullptr) return *this;
      if (ref->left != nullptr) {
        ref = ref->left;
        while (ref->right != nullptr) ref = ref->right;
        return *this;
      }
      while (ref->parent != nullptr && ref->parent->left == ref)
        ref = ref->parent;
      if (ref->parent != nullptr) ref = ref->parent;
      return *this;
    }

    const iterator_t operator++(int) {
      iterator_t t(*this);
      ++(*this);
      return t;
    }
    const iterator_t operator--(int) {
      iterator_t t(*this);
      --(*this);
      return t;
    }
    friend bool operator==(iterator_t const& a, iterator_t const& b) {
      return a.ref == b.ref;
    }
    friend bool operator!=(iterator_t const& a, iterator_t const& b) {
      return a.ref != b.ref;
    }
  };

 public:
  using value_type = T;
  using const_reference = T const&;
  using reference = T&;
  using const_pointer = T const*;
  using pointer = T*;

  using const_iterator = iterator_t<const T>;
  using iterator = iterator_t<const T>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  set() {}
  set(set const& other) : set() {
    dummy.left =
        other.dummy.left ? copy_tree(other.dummy.left, &dummy) : nullptr;
  }
  set& operator=(set other) {
    swap(*this, other);
    return *this;
  }
  ~set() { clear(); }

  bool empty() const noexcept { return dummy.left == nullptr; }
  void clear() noexcept {
    if (dummy.left) {
      dummy.left->destroy();
      dummy.left = nullptr;
    }
  }

  const_iterator begin() const noexcept {
    node* r = const_cast<node*>(&dummy);
    while (r->left) r = r->left;
    return const_iterator(r);
  }
  const_iterator end() const noexcept {
    return const_iterator(const_cast<node*>(&dummy));
  }
  iterator begin() noexcept {
    node* r = &dummy;
    while (r->left) r = r->left;
    return iterator(r);
  }
  iterator end() noexcept { return iterator(&dummy); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  iterator insert(const_iterator pos, const_reference v) {
    node_v* n = new node_v(v, pos.ref, pos.ref->parent);
    n->left->parent = n;
    n->parent->left = n;
    return iterator(*n);
  }

  iterator erase(const_iterator pos) {
    iterator r = pos;
    ++r;
    node* n = pos.ref;
    node*& parref = (n->parent->left == n ? n->parent->left : n->parent->right);
    if (!n->left && !n->right) {
      parref = nullptr;
    } else if (!n->right) {
      parref = n->left;
      n->left->parent = n->parent;
    } else if (!n->left) {
      parref = n->right;
      n->right->parent = n->parent;
    } else {
      node* p = r.ref;
      if (p->parent->left == p) {
        p->parent->left = p->right;
        if (p->right) p->right->parent = p->parent;
        p->right = nullptr;
        p->parent = nullptr;

        parref = p;
        p->parent = n->parent;
        p->left = n->left;
        n->left->parent = p;
        p->right = n->right;
        n->right->parent = p;
      } else {
        parref = p;
        p->parent = n->parent;
        p->left = n->left;
        n->left->parent = p;
      }
    }
    delete (static_cast<node_v*>(n));
    return r;
  }

  std::pair<iterator, bool> insert(const_reference v) {
    node* t = dummy.left;
    if (!t) {
      dummy.left = new node_v(v, &dummy);
      return {iterator(dummy.left), true};
    }
    while (true) {
      const_reference nv = static_cast<node_v*>(t)->value;
      if (nv == v) return {iterator(t), false};
      node*& cref = v < nv ? t->left : t->right;
      if (!cref) {
        cref = new node_v(v, t);
        return {iterator(cref), true};
      }
      t = cref;
    }
  }

  iterator upper_bound(T const& v) const {
    iterator r = lower_bound(v);
    return (r != end() && static_cast<node_v*>(r.ref)->value == v ? ++r : r);
  }

  iterator lower_bound(const_reference v) const {
    for (auto t = begin(); t != end(); ++t) {
      if (static_cast<node_v*>(t.ref)->value >= v) {
        return t;
      }
    }
    return end();
  }

  iterator find(const_reference v) const {
    if (empty()) return end();
    iterator r = lower_bound(v);
    return (r != end() && static_cast<node_v*>(r.ref)->value == v ? r : end());
  }

  template <typename V>
  friend void swap(set<V>&, set<V>&) noexcept;
};

template <typename V>
void swap(set<V>& a, set<V>& b) noexcept {
  if (!a.empty()) {
    a.dummy.left->parent = &b.dummy;
  }
  if (!b.empty()) {
    b.dummy.left->parent = &a.dummy;
  }
  std::swap(a.dummy, b.dummy);
}
