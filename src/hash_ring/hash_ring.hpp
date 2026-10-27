#pragma once

#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <shared_mutex>
#include <cstdint>
#include <optional>

namespace fashion::storage {

struct Node {
    std::string id;       // e.g., "node-1"
    std::string address;  // e.g., "127.0.0.1:8081"
};

class HashRing {
public:
    explicit HashRing(uint32_t num_vnodes = 100);

    // Node management
    void add_node(const Node& node);
    void remove_node(const std::string& node_id);

    // Key routing: returns the target physical Node for a tile key
    [[nodiscard]] std::optional<Node> get_node(std::string_view key) const;

    [[nodiscard]] size_t total_vnodes() const;

private:
    uint64_t hash_key(std::string_view key) const;

    uint32_t num_vnodes_;
    std::map<uint64_t, Node> ring_; // Hash ring mapping vnode_hash -> physical Node
    mutable std::shared_mutex ring_mutex_;
};

} // namespace fashion::storage