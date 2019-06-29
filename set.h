#pragma once
#include <memory>

template <typename T>
class set {
  struct node;
  using node_ptr = std::unique_ptr<node>;
  struct node {
    node_ptr left;
    node_ptr right;
    node* parent;
    node() : left(nullptr), right(nullptr), parent(nullptr) {}
    node(node* parent) : left(nullptr), right(nullptr), parent(parent) {}
    node(node* left, node* right, node* parent)
        : left(left), right(right), parent(parent) {}
    virtual ~node() = default;
  };

  struct node_v : public node {
    T value;
    node_v(T const& v) : node(), value(v) {}
    node_v(T const& v, node* parent) : node(parent), value(v) {}
    node_v(T const& v, node* left, node* right, node* parent)
        : node(left, right, parent), value(v) {}
  };

  node_ptr copy_tree(node* t, node* parent) {
    node_ptr n(new node_v(static_cast<node_v*>(t)->value, parent));
    if (t->left) n.get()->left = copy_tree(t->left.get(), n.get());
    if (t->right) n.get()->right = copy_tree(t->right.get(), n.get());
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
        ref = ref->right.get();
        while (ref->left != nullptr) ref = ref->left.get();
        return *this;
      }
      while (ref->parent != nullptr && ref->parent->right.get() == ref)
        ref = ref->parent;
      if (ref->parent != nullptr) ref = ref->parent;
      return *this;
    }
    iterator_t& operator--() {
      if (ref == nullptr) return *this;
      if (ref->left != nullptr) {
        ref = ref->left.get();
        while (ref->right != nullptr) ref = ref->right.get();
        return *this;
      }
      while (ref->parent != nullptr && ref->parent->left.get() == ref)
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
        other.dummy.left ? copy_tree(other.dummy.left.get(), &dummy) : nullptr;
  }
  set& operator=(set other) {
    swap(*this, other);
    return *this;
  }
  ~set() { clear(); }

  bool empty() const noexcept { return dummy.left == nullptr; }
  void clear() noexcept {
    if (dummy.left) {
      dummy.left.reset();
    }
  }

  const_iterator begin() const noexcept {
    node* r = const_cast<node*>(&dummy);
    while (r->left) r = r->left.get();
    return const_iterator(r);
  }
  const_iterator end() const noexcept {
    return const_iterator(const_cast<node*>(&dummy));
  }
  iterator begin() noexcept {
    node* r = &dummy;
    while (r->left) r = r->left.get();
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
    node_ptr& parref =
        (pos.ref->parent->left.get() == pos.ref ? pos.ref->parent->left
                                                : pos.ref->parent->right);
    node* n = parref.release();
    if (!n->left && !n->right) {
      parref.reset();
    } else if (!n->right) {
      n->left.get()->parent = n->parent;
      parref.swap(n->left);
    } else if (!n->left) {
      n->right.get()->parent = n->parent;
      parref.swap(n->right);
    } else {
      bool was_p_left = r.ref->parent->left.get() == r.ref;
      node* p =
          (was_p_left ? r.ref->parent->left : r.ref->parent->right).release();
      if (was_p_left) {
        if (p->right != nullptr) p->right.get()->parent = p->parent;
        p->parent->left.swap(p->right);

        p->parent = n->parent;
        n->left.get()->parent = p;
        n->right.get()->parent = p;
        p->left.swap(n->left);
        p->right.swap(n->right);
        parref = node_ptr(p);
      } else {
        p->parent = n->parent;
        n->left.get()->parent = p;
        p->left.swap(n->left);
        parref = node_ptr(p);
      }
    }
    delete n;
    return r;
  }

  std::pair<iterator, bool> insert(const_reference v) {
    node* t = dummy.left.get();
    if (!t) {
      dummy.left = node_ptr(new node_v(v, &dummy));
      return {iterator(dummy.left.get()), true};
    }
    while (true) {
      const_reference nv = static_cast<node_v*>(t)->value;
      if (nv == v) return {iterator(t), false};
      node_ptr& cref = v < nv ? t->left : t->right;
      if (cref == nullptr) {
        cref = node_ptr(new node_v(v, t));
        return {iterator(cref.get()), true};
      }
      t = cref.get();
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
  std::swap(a.dummy.left, b.dummy.left);
}
