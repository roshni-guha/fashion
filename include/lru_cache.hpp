#pragma once

#include <string>
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <mutex>
#include <optional>
#include <cstddef>

namespace fashion::storage {

class LRUCache {
public:
    explicit LRUCache(size_t capacity_bytes) 
        : max_capacity_(capacity_bytes), current_size_(0) {}

    std::optional<std::string> get(const std::string& key) {
        std::unique_lock lock(mutex_); // Unique lock required to reorder LRU list on hit
        auto it = index_.find(key);
        if (it == index_.end()) {
            return std::nullopt;
        }

        // Splice moves the accessed item to the front of the list in O(1)
        items_.splice(items_.begin(), items_, it->second);
        return it->second->second;
    }

    void put(const std::string& key, std::string value) {
        std::unique_lock lock(mutex_);
        size_t value_size = value.size();

        auto it = index_.find(key);
        if (it != index_.end()) {
            current_size_ -= it->second->second.size();
            items_.erase(it->second);
            index_.erase(it);
        }

        // Evict least-recently used elements from the back if over capacity
        while (current_size_ + value_size > max_capacity_ && !items_.empty()) {
            auto& last = items_.back();
            current_size_ -= last.second.size();
            index_.erase(last.first);
            items_.pop_back();
        }

        // If the single tile exceeds total cache capacity, do not cache it
        if (value_size > max_capacity_) {
            return;
        }

        items_.emplace_front(key, std::move(value));
        index_[key] = items_.begin();
        current_size_ += value_size;
    }

    size_t byte_size() const {
        std::shared_lock lock(mutex_);
        return current_size_;
    }

private:
    using ItemList = std::list<std::pair<std::string, std::string>>;

    size_t max_capacity_;
    size_t current_size_;
    ItemList items_;
    std::unordered_map<std::string, ItemList::iterator> index_;
    mutable std::shared_mutex mutex_;
};

} // namespace fashion::storage