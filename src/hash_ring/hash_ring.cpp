#include "hash_ring.hpp"
#include <functional> // For std::hash
#include <mutex>      // For std::unique_lock, std::shared_lock

namespace fashion::storage {

HashRing::HashRing(uint32_t num_vnodes) : num_vnodes_(num_vnodes) {}

uint64_t HashRing::hash_key(std::string_view key) const {
    // std::hash is our baseline; in a strict Quant/ULL environment, 
    // we would swap this for MurmurHash3 or CityHash for better distribution.
    return std::hash<std::string_view>{}(key);
}

void HashRing::add_node(const Node& node) {
    // Exclusive lock: Block all readers/writers while we mutate the ring
    std::unique_lock lock(ring_mutex_);
    
    for (uint32_t i = 0; i < num_vnodes_; ++i) {
        // Create a unique virtual node string, e.g., "node-1#0", "node-1#1"
        std::string vnode_key = node.id + "#" + std::to_string(i);
        uint64_t hash_val = hash_key(vnode_key);
        
        // Insert the physical node into the map at the virtual node's hash position
        ring_[hash_val] = node;
    }
}

void HashRing::remove_node(const std::string& node_id) {
    std::unique_lock lock(ring_mutex_);
    
    for (uint32_t i = 0; i < num_vnodes_; ++i) {
        std::string vnode_key = node_id + "#" + std::to_string(i);
        uint64_t hash_val = hash_key(vnode_key);
        
        ring_.erase(hash_val);
    }
}

std::optional<Node> HashRing::get_node(std::string_view key) const {
    // Shared lock: Allow infinite parallel readers, only block if a node is being added/removed
    std::shared_lock lock(ring_mutex_);
    
    if (ring_.empty()) {
        return std::nullopt;
    }
    
    uint64_t hash_val = hash_key(key);
    
    // Binary search for the first node whose hash is >= our key's hash
    auto it = ring_.lower_bound(hash_val);
    
    // If we walked past the end of the map, wrap around to the first node (forming the "ring")
    if (it == ring_.end()) {
        it = ring_.begin();
    }
    
    return it->second;
}

size_t HashRing::total_vnodes() const {
    std::shared_lock lock(ring_mutex_);
    return ring_.size();
}

} // namespace fashion::storage