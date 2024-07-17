#include <iostream>

template <typename T, typename Allocator = std::allocator<T>>
class List {
 public:
  List() = default;
  List(size_t count, const T& value, const Allocator& alloc = Allocator());
  explicit List(size_t count, const Allocator& alloc = Allocator());
  List(const List& other);
  List(std::initializer_list<T> init, const Allocator& alloc = Allocator());

  ~List();

  List& operator=(const List& other);

  T& front();
  const T& front() const;
  T& back();
  const T& back() const;

  bool empty() const;
  size_t size() const;

  void push_back(const T& element);
  void push_front(const T& element);
  void pop_back();
  void pop_front();

  template <bool IsConst>
  class Iterator;

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<Iterator<false>>;
  using const_reverse_iterator = std::reverse_iterator<Iterator<true>>;

  iterator begin() const;
  const_iterator cbegin() const;
  iterator end() const;
  const_iterator cend() const;
  reverse_iterator rbegin() const;
  reverse_iterator rend() const;
  const_reverse_iterator crbegin() const;
  const_reverse_iterator crend() const;

  using value_type = T;

  struct BaseNode {
    BaseNode() = default;
    BaseNode* prev = this;
    BaseNode* next = this;
  };

  struct Node : BaseNode {
    Node() = default;
    explicit Node(const value_type& value) : value(value) {}
    value_type value;
  };

  using allocator_type = Allocator;
  using node_allocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using node_allocator_traits = std::allocator_traits<node_allocator>;

  node_allocator get_allocator() const { return alloc_; }

 private:
  void clear();

  BaseNode fake_node_;
  size_t size_ = 0;
  node_allocator alloc_;
};

template <typename T, typename Allocator>
void List<T, Allocator>::clear() {
  BaseNode* cur_node = fake_node_.next;
  for (size_t i = 0; i < size_; ++i) {
    BaseNode* next_node = cur_node->next;
    node_allocator_traits::destroy(alloc_, static_cast<Node*>(cur_node));
    node_allocator_traits::deallocate(alloc_, static_cast<Node*>(cur_node), 1);
    cur_node = next_node;
  }
}

template <typename T, typename Alloc>
List<T, Alloc>::List(size_t count, const T& value, const Alloc& alloc)
    : size_(count), alloc_(alloc) {
  BaseNode* cur_node = &fake_node_;
  for (size_t cur_index = 0; cur_index < count; ++cur_index) {
    cur_node->next = node_allocator_traits::allocate(alloc_, 1);
    try {
      node_allocator_traits::construct(
          alloc_, static_cast<Node*>(cur_node->next), value);
    } catch (...) {
      node_allocator_traits::deallocate(alloc_,
                                        static_cast<Node*>(cur_node->next), 1);
      for (size_t i = 0; i < cur_index; ++i) {
        BaseNode* prev_node = cur_node->prev;
        node_allocator_traits::destroy(alloc_,
                                       static_cast<Node*>(prev_node->next));
        node_allocator_traits::deallocate(
            alloc_, static_cast<Node*>(prev_node->next), 1);
        cur_node = prev_node;
      }
      size_ = 0;
      throw;
    }
    cur_node->next->prev = cur_node;
    cur_node = cur_node->next;
  }
  cur_node->next = &fake_node_;
  fake_node_.prev = cur_node;
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t count, const Allocator& alloc)
    : size_(count), alloc_(alloc) {
  BaseNode* cur_node = &fake_node_;
  for (size_t cur_index = 0; cur_index < count; ++cur_index) {
    cur_node->next = node_allocator_traits::allocate(alloc_, 1);
    try {
      node_allocator_traits::construct(alloc_,
                                       static_cast<Node*>(cur_node->next));
    } catch (...) {
      node_allocator_traits::deallocate(alloc_,
                                        static_cast<Node*>(cur_node->next), 1);
      for (size_t i = 0; i < cur_index; ++i) {
        BaseNode* prev_node = cur_node->prev;
        node_allocator_traits::destroy(alloc_,
                                       static_cast<Node*>(prev_node->next));
        node_allocator_traits::deallocate(
            alloc_, static_cast<Node*>(prev_node->next), 1);
        cur_node = prev_node;
      }
      size_ = 0;
      throw;
    }
    cur_node->next->prev = cur_node;
    cur_node = cur_node->next;
  }
  cur_node->next = &fake_node_;
  fake_node_.prev = cur_node;
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List& other) {
  alloc_ = std::allocator_traits<
      node_allocator>::select_on_container_copy_construction(other.alloc_);
  size_t cur_index = 0;
  try {
    BaseNode* other_cur_node = other.fake_node_.next;
    for (; cur_index < other.size_; ++cur_index) {
      push_back(static_cast<Node*>(other_cur_node)->value);
      other_cur_node = other_cur_node->next;
    }
  } catch (...) {
    for (size_t i = 0; i < cur_index; ++i) {
      pop_back();
    }
    throw;
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(std::initializer_list<T> init,
                         const Allocator& alloc) {
  alloc_ = std::allocator_traits<
      node_allocator>::select_on_container_copy_construction(alloc);
  size_t cur_index = 0;
  try {
    for (const T& elem : init) {
      push_back(elem);
    }
  } catch (...) {
    for (size_t i = 0; i < cur_index; ++i) {
      pop_back();
    }
    throw;
  }
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& other) {
  if (this != &other) {
    node_allocator next_allocator = alloc_;
    node_allocator old_allocator = alloc_;
    size_t old_size = size_;
    if (std::allocator_traits<
            node_allocator>::propagate_on_container_copy_assignment::value) {
      next_allocator = other.alloc_;
    }
    size_t cur_index = 0;
    try {
      alloc_ = next_allocator;
      BaseNode* other_cur_node = other.fake_node_.next;
      for (; cur_index < other.size_; ++cur_index) {
        push_back(static_cast<Node*>(other_cur_node)->value);
        other_cur_node = other_cur_node->next;
      }
    } catch (...) {
      for (size_t i = 0; i < cur_index; ++i) {
        pop_back();
      }
      alloc_ = old_allocator;
      throw;
    }
    alloc_ = old_allocator;
    for (size_t i = 0; i < old_size; ++i) {
      pop_front();
    }
    alloc_ = next_allocator;
  }
  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>::~List() {
  clear();
}

template <typename T, typename Allocator>
T& List<T, Allocator>::front() {
  return static_cast<Node*>(fake_node_.next)->value;
}

template <typename T, typename Allocator>
const T& List<T, Allocator>::front() const {
  return static_cast<Node*>(fake_node_.next)->value;
}

template <typename T, typename Allocator>
T& List<T, Allocator>::back() {
  return static_cast<Node*>(fake_node_.prev)->value;
}

template <typename T, typename Allocator>
const T& List<T, Allocator>::back() const {
  return static_cast<Node*>(fake_node_.prev)->value;
}

template <typename T, typename Allocator>
bool List<T, Allocator>::empty() const {
  return size_ == 0;
}

template <typename T, typename Allocator>
size_t List<T, Allocator>::size() const {
  return size_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& element) {
  BaseNode* last_node;
  last_node = node_allocator_traits::allocate(alloc_, 1);
  try {
    node_allocator_traits::construct(alloc_, static_cast<Node*>(last_node),
                                     element);
  } catch (...) {
    node_allocator_traits::deallocate(alloc_, static_cast<Node*>(last_node), 1);
    throw;
  }
  fake_node_.prev->next = last_node;
  last_node->prev = fake_node_.prev;
  last_node->next = &fake_node_;
  fake_node_.prev = last_node;
  ++size_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& element) {
  BaseNode* first_node;
  first_node = node_allocator_traits::allocate(alloc_, 1);
  try {
    node_allocator_traits::construct(alloc_, static_cast<Node*>(first_node),
                                     element);
  } catch (...) {
    node_allocator_traits::deallocate(alloc_, static_cast<Node*>(first_node),
                                      1);
    throw;
  }
  fake_node_.next->prev = first_node;
  first_node->next = fake_node_.next;
  first_node->prev = &fake_node_;
  fake_node_.next = first_node;
  ++size_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  BaseNode* node_before_last = fake_node_.prev->prev;
  node_allocator_traits::destroy(alloc_,
                                 static_cast<Node*>(node_before_last->next));
  node_allocator_traits::deallocate(
      alloc_, static_cast<Node*>(node_before_last->next), 1);
  node_before_last->next = &fake_node_;
  fake_node_.prev = node_before_last;
  --size_;
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  BaseNode* node_after_first = fake_node_.next->next;
  node_allocator_traits::destroy(alloc_,
                                 static_cast<Node*>(node_after_first->prev));
  node_allocator_traits::deallocate(
      alloc_, static_cast<Node*>(node_after_first->prev), 1);
  node_after_first->prev = &fake_node_;
  fake_node_.next = node_after_first;
  --size_;
}

template <typename T, typename Allocator>
template <bool IsConst>
class List<T, Allocator>::Iterator {
 public:
  using iterator_category = std::bidirectional_iterator_tag;
  using cond_type = std::conditional_t<IsConst, const T, T>;
  using value_type = cond_type;
  using pointer = cond_type*;
  using reference = cond_type&;
  using difference_type = std::ptrdiff_t;

  Iterator(const BaseNode& node) : cur_node_(const_cast<BaseNode*>(&node)) {}
  Iterator(const Iterator& other) = default;
  Iterator& operator=(const Iterator& other) = default;

  Iterator& operator++() {
    cur_node_ = cur_node_->next;
    return *this;
  }
  Iterator& operator--() {
    cur_node_ = cur_node_->prev;
    return *this;
  }
  Iterator operator++(int) {
    Iterator<IsConst> tmp = *this;
    ++(*this);
    return tmp;
  }
  Iterator operator--(int) {
    Iterator<IsConst> tmp = *this;
    --(*this);
    return tmp;
  }

  reference operator*() const { return static_cast<Node*>(cur_node_)->value; }

  pointer operator->() const { return &(static_cast<Node*>(cur_node_)->value); }

  bool operator==(const Iterator& other) const {
    return (this->cur_node_ == other.cur_node_);
  }

  bool operator!=(const Iterator& other) const { return !(*this == other); }

 private:
  BaseNode* cur_node_;
};

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() const {
  return List::iterator(*fake_node_.next);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() const {
  return List::iterator(fake_node_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const {
  return List::const_iterator(*fake_node_.next);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cend() const {
  return List::const_iterator(fake_node_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rbegin()
    const {
  return std::make_reverse_iterator(end());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rend() const {
  return std::make_reverse_iterator(begin());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator
List<T, Allocator>::crbegin() const {
  return std::make_reverse_iterator(cend());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::crend()
    const {
  return std::make_reverse_iterator(cbegin());
}