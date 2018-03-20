#pragma once

#include <algorithm>
#include <cassert>
#include <limits>
#include <memory>
#include <vector>

// Cache that evicts old entries which have not been used recently. Implemented
// using array/linear search so this works well for small array sizes.
template <typename TKey, typename TValue>
struct LruCache {
  explicit LruCache(int max_entries);

  // Fetches an entry for |key|. If it does not exist, |allocator| will be
  // invoked to create one.
  template <typename TAllocator>
  std::shared_ptr<TValue> Get(const TKey& key, TAllocator allocator);
  // Fetches the entry for |filename| and updates it's usage so it is less
  // likely to be evicted.
  std::shared_ptr<TValue> TryGet(const TKey& key);
  // TryGetEntry, except the entry is removed from the cache.
  std::shared_ptr<TValue> TryTake(const TKey& key);
  // Inserts an entry. Evicts the oldest unused entry if there is no space.
  void Insert(const TKey& key, const std::shared_ptr<TValue>& value);

  // Call |func| on existing entries. If |func| returns false iteration
  // temrinates early.
  template <typename TFunc>
  void IterateValues(TFunc func);

  // Empties the cache
  void Clear(void);

 private:
  // There is a global score counter, when we access an element we increase
  // its score to the current global value, so it has the highest overall
  // score. This means that the oldest/least recently accessed value has the
  // lowest score.
  //
  // There is a bit of special logic to handle score overlow.
  struct Entry {
    uint32_t score = 0;
    TKey key;
    std::shared_ptr<TValue> value;
    bool operator<(const Entry& other) const { return score < other.score; }
  };

  void IncrementScore();

  std::vector<Entry> entries_;
  int max_entries_ = 1;
  uint32_t next_score_ = 0;
};

template <typename TKey, typename TValue>
LruCache<TKey, TValue>::LruCache(int max_entries) : max_entries_(max_entries) {
  assert(max_entries > 0);
}

template <typename TKey, typename TValue>
template <typename TAllocator>
std::shared_ptr<TValue> LruCache<TKey, TValue>::Get(const TKey& key,
                                                    TAllocator allocator) {
  std::shared_ptr<TValue> result = TryGet(key);
  if (!result)
    Insert(key, result = allocator());
  return result;
}

template <typename TKey, typename TValue>
std::shared_ptr<TValue> LruCache<TKey, TValue>::TryGet(const TKey& key) {
  // Assign new score.
  for (Entry& entry : entries_) {
    if (entry.key == key) {
      entry.score = next_score_;
      IncrementScore();
      return entry.value;
    }
  }

  return nullptr;
}

template <typename TKey, typename TValue>
std::shared_ptr<TValue> LruCache<TKey, TValue>::TryTake(const TKey& key) {
  for (size_t i = 0; i < entries_.size(); ++i) {
    if (entries_[i].key == key) {
      std::shared_ptr<TValue> copy = entries_[i].value;
      entries_.erase(entries_.begin() + i);
      return copy;
    }
  }

  return nullptr;
}

template <typename TKey, typename TValue>
void LruCache<TKey, TValue>::Insert(const TKey& key,
                                    const std::shared_ptr<TValue>& value) {
  if ((int)entries_.size() >= max_entries_)
    entries_.erase(std::min_element(entries_.begin(), entries_.end()));

  Entry entry;
  entry.score = next_score_;
  IncrementScore();
  entry.key = key;
  entry.value = value;
  entries_.push_back(entry);
}

template <typename TKey, typename TValue>
template <typename TFunc>
void LruCache<TKey, TValue>::IterateValues(TFunc func) {
  for (Entry& entry : entries_) {
    if (!func(entry.value))
      break;
  }
}

template <typename TKey, typename TValue>
void LruCache<TKey, TValue>::IncrementScore() {
  // Overflow.
  if (++next_score_ == 0) {
    std::sort(entries_.begin(), entries_.end());
    for (Entry& entry : entries_)
      entry.score = next_score_++;
  }
}

template <typename TKey, typename TValue>
void LruCache<TKey, TValue>::Clear(void) {
	entries_.clear();
	next_score_ = 0;
}
