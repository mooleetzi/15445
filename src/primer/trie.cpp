#include "primer/trie.h"
#include <string_view>
#include "common/exception.h"

namespace bustub {

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  std::shared_ptr<const TrieNode> cur = root_;
  for (char c : key) {
    if (!cur->children_.count(c)) return nullptr;
    cur = cur->children_.at(c);
  }
  if (!cur->is_value_node_) return nullptr;
  auto t = dynamic_cast<const TrieNodeWithValue<T> *>(cur.get());
  if (!t) return nullptr;
  return t->value_.get();
  // You should walk through the trie to find the node corresponding to the key. If the node doesn't exist, return
  // nullptr. After you find the node, you should use `dynamic_cast` to cast it to `const TrieNodeWithValue<T> *`. If
  // dynamic_cast returns `nullptr`, it means the type of the value is mismatched, and you should return nullptr.
  // Otherwise, return the value.
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  if (key.empty()) {
    if (!root_) return Trie{std::make_shared<const TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)))};
    return Trie{std::make_shared<const TrieNodeWithValue<T>>(root_->children_, std::make_shared<T>(std::move(value)))};
  }

  std::shared_ptr<TrieNode> newRoot = nullptr;

  if (root_) newRoot = root_->Clone();

  if (!newRoot) newRoot = std::make_shared<TrieNode>();

  auto now = newRoot;

  for (char c : key.substr(0, key.size() - 1)) {
    if (!now->children_.count(c))
      now->children_[c] = std::make_shared<const TrieNode>();
    else
      now->children_[c] = now->children_[c]->Clone();
    now = std::const_pointer_cast<TrieNode>(now->children_[c]);
  }
  char c = key.back();
  if (!now->children_.count(c))
    now->children_[c] = std::make_shared<const TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)));
  else
    now->children_[c] = std::make_shared<const TrieNodeWithValue<T>>(now->children_[c]->children_,
                                                                     std::make_shared<T>(std::move(value)));
  return Trie{newRoot};

  // You should walk through the trie and create new nodes if necessary. If the node corresponding to the key already
  // exists, you should create a new `TrieNodeWithValue`.
}
auto Trie::Remove(std::string_view key) const -> Trie {
  if (!root_) return *this;

  if (key.empty()) {
    if (root_->is_value_node_) return Trie{std::make_shared<const TrieNode>(root_->children_)};
    return *this;
  }

  std::shared_ptr<TrieNode> newRoot = root_->Clone();
  auto now = newRoot;
  for (char c : key.substr(0, key.size() - 1)) {
    if (!now->children_.count(c)) return Trie{newRoot};
    now->children_[c] = now->children_[c]->Clone();
    now = std::const_pointer_cast<TrieNode>(now->children_[c]);
  }
  char c = key.back();
  if (!now->children_.count(c)) return Trie{newRoot};
  now->children_[c] = now->children_[c]->Clone();

  auto del = now->children_[c];

  if (del->children_.empty()) {
    now->children_.erase(c);
  } else if (del->is_value_node_) {
    now->children_[c] = std::make_shared<const TrieNode>(del->children_);
  }
  return Trie{newRoot};

  // You should walk through the trie and remove nodes if necessary. If the node doesn't contain a value any more,
  // you should convert it to `TrieNode`. If a node doesn't have children any more, you should remove it.
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked up
// by the linker.

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
