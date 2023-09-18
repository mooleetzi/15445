//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include "common/config.h"
#include "common/exception.h"

namespace bustub {

LRUKNode::LRUKNode(size_t k, frame_id_t fid, size_t timestamp) : k_(k), fid_(fid) { history_.push_front(timestamp); }
LRUKNode::~LRUKNode() { history_.clear(); }
void LRUKNode::DropLinks() {
  auto next = this->next_;
  auto prev = this->prev_;
  if (next != nullptr) {
    next->prev_ = prev;
  }
  if (prev != nullptr) {
    prev->next_ = next;
  }
  this->next_ = nullptr;
  this->prev_ = nullptr;
}

LRUKContainer::LRUKContainer(const std::shared_ptr<LRUKContainer> &other) {
  if (other) {
    this->other_ = other;
    this->relocate_when_need_ = true;
  }
}
LRUKContainer::~LRUKContainer() { node_store_.clear(); }

auto LRUKContainer::FindNode(frame_id_t fid) -> std::shared_ptr<LRUKNode> {
  auto it = node_store_.find(fid);
  if (it == node_store_.end()) {
    return nullptr;
  }
  return it->second;
}

void LRUKContainer::AddNode(const std::shared_ptr<LRUKNode> &node) {
  if (node->k_ == node->history_.size() && relocate_when_need_) {
    other_.lock()->AddNode(node);
    return;
  }
  node_store_.insert(std::make_pair(node->fid_, node));
  if (head_ == nullptr) {
    head_ = tail_ = node;
    return;
  }
  node->prev_ = tail_;
  tail_->next_ = node;
  tail_ = tail_->next_;
}
void LRUKContainer::RemoveNode(const std::shared_ptr<LRUKNode> &node) {
  auto next = node->next_;
  auto prev = node->prev_;
  node->DropLinks();
  if (head_ == node) {
    head_ = next;
  }
  if (tail_ == node) {
    tail_ = prev;
  }
  node_store_.erase(node->fid_);
}

void LRUKContainer::UpdateNode(const std::shared_ptr<LRUKNode> &node, size_t timestamp) {
  if (node->k_ <= node->history_.size() + 1 && relocate_when_need_) {
    RemoveNode(node);
    other_.lock()->UpdateNode(node, timestamp);
    return;
  }

  if (node->history_.size() == node->k_) {
    node->history_.pop_back();
  }
  node->history_.push_front(timestamp);

  if (node_store_.count(node->fid_) == 0) {
    AddNode(node);
  }

  if (node == tail_) {
    return;
  }
  auto next = node->next_;
  node->DropLinks();
  if (head_ == node) {
    head_ = next;
  }
  tail_->next_ = node;
  node->prev_ = tail_;
  tail_ = node;
}

auto LRUKContainer::Evict(frame_id_t *fid) -> bool {
  auto cur = head_;
  while (cur != nullptr) {
    if (cur->is_evictable_) {
      *fid = cur->fid_;
      RemoveNode(cur);
      return true;
    }
    cur = cur->next_;
  }
  return false;
}

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {
  ctr_old_ = std::make_shared<LRUKContainer>(nullptr);
  ctr_young_ = std::make_shared<LRUKContainer>(ctr_old_);
}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::lock_guard<std::mutex> lock(latch_);
  if (ctr_young_->Evict(frame_id) || ctr_old_->Evict(frame_id)) {
    --curr_size_;
    return true;
  }
  return false;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  if (static_cast<decltype(replacer_size_)>(frame_id) >= replacer_size_) {
    throw std::runtime_error("frame id shoule less than replacer_size_");
  }

  time_t timestamp;
  time(&timestamp);

  std::lock_guard<std::mutex> lock(latch_);
  current_timestamp_ = static_cast<size_t>(timestamp);

  auto node = ctr_young_->FindNode(frame_id);
  if (node != nullptr) {
    ctr_young_->UpdateNode(node, current_timestamp_);
    return;
  }
  node = ctr_old_->FindNode(frame_id);
  if (node != nullptr) {
    ctr_old_->UpdateNode(node, current_timestamp_);
    return;
  }
  node = std::make_shared<LRUKNode>(k_, frame_id, current_timestamp_);
  ctr_young_->AddNode(node);
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  if (static_cast<decltype(replacer_size_)>(frame_id) >= replacer_size_) {
    throw std::runtime_error("frame id shoule less than replacer_size_");
  }
  std::lock_guard<std::mutex> lock(latch_);
  auto node = ctr_young_->FindNode(frame_id);
  if (node == nullptr) {
    node = ctr_old_->FindNode(frame_id);
  }

  if (node == nullptr || node->is_evictable_ == set_evictable) {
    return;
  }

  if (node->is_evictable_ && !set_evictable) {
    node->is_evictable_ = set_evictable;
    --curr_size_;
    return;
  }
  node->is_evictable_ = set_evictable;
  ++curr_size_;
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  if (static_cast<decltype(replacer_size_)>(frame_id) >= replacer_size_) {
    throw std::runtime_error("lru-k-replacer: frame id shoule less than replacer_size_");
  }
  std::lock_guard<std::mutex> lock(latch_);

  auto node = ctr_young_->FindNode(frame_id);
  if (node != nullptr) {
    if (!node->is_evictable_) {
      throw std::runtime_error("lru-k-replacer: cannot remove non-evictable frame");
    }
    --curr_size_;
    ctr_young_->RemoveNode(node);
    return;
  }
  node = ctr_old_->FindNode(frame_id);
  if (node != nullptr) {
    if (!node->is_evictable_) {
      throw std::runtime_error("lru-k-replacer: cannot remove non-evictable frame");
    }
    --curr_size_;
    ctr_old_->RemoveNode(node);
    return;
  }
}

auto LRUKReplacer::Size() -> size_t {
  std::lock_guard<std::mutex> lock(latch_);
  return curr_size_;
}

LRUKReplacer::~LRUKReplacer() = default;

}  // namespace bustub
