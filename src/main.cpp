#include "hash_ring/hash_ring.hpp"
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <string>

using namespace fashion::storage;

int main() {
    // 1. Initialize the hash ring with 100 virtual nodes per physical server
    HashRing ring(100);

    // 2. Bring three physical storage nodes online
    ring.add_node({"node-A", "10.0.0.1:8080"});
    ring.add_node({"node-B", "10.0.0.2:8080"});
    ring.add_node({"node-C", "10.0.0.3:8080"});

    std::cout << "Total virtual nodes on ring: " << ring.total_vnodes() << "\n\n";

    // 3. Simulate routing 100,000 Deep-Zoom image tiles
    std::unordered_map<std::string, int> load_distribution;
    const int total_tiles = 100000;

    for (int i = 0; i < total_tiles; ++i) {
        std::string tile_key = "gucci_dress_tile_" + std::to_string(i) + ".png";
        
        auto target_node = ring.get_node(tile_key);
        if (target_node) {
            load_distribution[target_node->id]++;
        }
    }

    // 4. Print the distribution results
    std::cout << "--- Initial Load Distribution ---\n";
    for (const auto& [node_id, count] : load_distribution) {
        double percentage = (count / static_cast<double>(total_tiles)) * 100.0;
        std::cout << node_id << " holds " << count << " tiles (" 
                  << std::fixed << std::setprecision(2) << percentage << "%)\n";
    }

    // 5. Simulate a server crash (node-C goes offline)
    std::cout << "\n[Alert] node-C crashed. Removing from ring...\n\n";
    ring.remove_node("node-C");

    // 6. Recalculate where the tiles route now
    load_distribution.clear();
    for (int i = 0; i < total_tiles; ++i) {
        std::string tile_key = "gucci_dress_tile_" + std::to_string(i) + ".png";
        auto target_node = ring.get_node(tile_key);
        if (target_node) {
            load_distribution[target_node->id]++;
        }
    }

    std::cout << "--- Load Distribution After Crash ---\n";
    for (const auto& [node_id, count] : load_distribution) {
        double percentage = (count / static_cast<double>(total_tiles)) * 100.0;
        std::cout << node_id << " holds " << count << " tiles (" 
                  << std::fixed << std::setprecision(2) << percentage << "%)\n";
    }

    return 0;
}