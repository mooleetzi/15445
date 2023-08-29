#include "primer/trie.h"
#include <iostream>
#include <memory>
#include <stack>
#include <string_view>
#include <utility>
#include "common/exception.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  if (!root_) {
    return nullptr;
  }
  std::shared_ptr<const TrieNode> cur = root_;
  for (auto chr : key) {
    if (!cur->children_.count(chr)) {
      return nullptr;
    }
    cur = cur->children_.at(chr);
  }
  if (!cur->is_value_node_) {
    return nullptr;
  }
  auto value_ptr = dynamic_cast<const TrieNodeWithValue<T> *>(cur.get());
  if (!value_ptr) {
    return nullptr;
  }
  return value_ptr->value_.get();
  // You should walk through the trie to find the node corresponding to the key. If the node doesn't exist, return
  // nullptr. After you find the node, you should use `dynamic_cast` to cast it to `const TrieNodeWithValue<T> *`. If
  // dynamic_cast returns `nullptr`, it means the type of the value is mismatched, and you should return nullptr.
  // Otherwise, return the value.
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  if (key.empty()) {
    if (!root_) {
      return Trie{std::make_shared<const TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)))};
    }
    return Trie{std::make_shared<const TrieNodeWithValue<T>>(root_->children_, std::make_shared<T>(std::move(value)))};
  }

  std::shared_ptr<TrieNode> new_root = nullptr;

  if (root_) {
    new_root = root_->Clone();
  }

  if (!new_root) {
    new_root = std::make_shared<TrieNode>();
  }

  auto now = new_root;

  for (char chr : key.substr(0, key.size() - 1)) {
    if (!now->children_.count(chr)) {
      now->children_[chr] = std::make_shared<const TrieNode>();
    } else {
      now->children_[chr] = now->children_[chr]->Clone();
    }
    now = std::const_pointer_cast<TrieNode>(now->children_[chr]);
  }
  char chr = key.back();
  if (!now->children_.count(chr)) {
    now->children_[chr] = std::make_shared<const TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)));
  } else {
    now->children_[chr] = std::make_shared<const TrieNodeWithValue<T>>(now->children_[chr]->children_,
                                                                       std::make_shared<T>(std::move(value)));
  }
  return Trie{new_root};

  // You should walk through the trie and create new nodes if necessary. If the node corresponding to the key already
  // exists, you should create a new `TrieNodeWithValue`.
}

auto Trie::ClearNode(std::stack<std::shared_ptr<TrieNode>> stack, std::string_view key) const
    -> std::shared_ptr<TrieNode> {
  std::shared_ptr<TrieNode> ret = nullptr;
  bool erase = true;
  for (auto it = key.rbegin(); it != key.rend(); it++) {
    ret = stack.top();
    stack.pop();
    if (erase) {
      ret->children_.erase(*it);
    }
    erase = false;
    if (ret->children_.empty() && !ret->is_value_node_) {
      erase = true;
    }
  }
  if (erase) {
    return nullptr;
  }
  return ret;
}
auto Trie::Remove(std::string_view key) const -> Trie {
  if (!root_) {
    return *this;
  }

  if (key.empty()) {
    if (root_->is_value_node_) {
      return Trie{std::make_shared<const TrieNode>(root_->children_)};
    }
    return *this;
  }

  std::stack<std::shared_ptr<TrieNode>> st;
  std::shared_ptr<TrieNode> new_root = root_->Clone();
  auto now = new_root;
  for (char chr : key.substr(0, key.size() - 1)) {
    if (now->children_.count(chr) == 0U) {
      return Trie{new_root};
    }
    st.emplace(now);
    now->children_[chr] = now->children_[chr]->Clone();
    now = std::const_pointer_cast<TrieNode>(now->children_[chr]);
  }
  char chr = key.back();
  if (now->children_.count(chr) == 0U) {
    return Trie{new_root};
  }
  st.emplace(now);
  now->children_[chr] = now->children_[chr]->Clone();

  auto del = now->children_[chr];
  if (del->children_.empty()) {
    new_root = ClearNode(std::move(st), key);
  } else if (del->is_value_node_) {
    now->children_[chr] = std::make_shared<const TrieNode>(del->children_);
  }

  return Trie{new_root};

  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value any more,
  // you should convert it to `TrieNode`. If a node doesn't have children any more, you should remove it.
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked
// up by the linker.

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

// If your solution cannot compile for non-copy tests, you can remove the below lines to get partial score.

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;

}  // namespace bustub
