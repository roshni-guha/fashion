#include "../../include/httplib.h"
#include "../../include/lru_cache.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

using namespace fashion::storage;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <dataset_directory>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string base_dir = argv[2];

    // Allocate 64 MB of RAM per storage node for the in-memory tile cache
    constexpr size_t CACHE_SIZE = 64 * 1024 * 1024;
    LRUCache tile_cache(CACHE_SIZE);

    httplib::Server svr;

    svr.Get(R"(/tiles/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string tile_path = req.matches[1];

        // 1. Check in-memory LRU cache (L1 Hit)
        auto cached = tile_cache.get(tile_path);
        if (cached) {
            res.set_content(*cached, "image/png");
            res.set_header("X-Cache", "HIT");
            return;
        }

        // 2. Cache Miss: Fall back to disk I/O
        std::filesystem::path full_path = std::filesystem::path(base_dir) / tile_path;
        if (!std::filesystem::exists(full_path)) {
            res.status = 404;
            res.set_content("Tile not found", "text/plain");
            return;
        }

        std::ifstream file(full_path, std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
            
            // Populate cache for subsequent requests
            tile_cache.put(tile_path, content);

            res.set_content(content, "image/png");
            res.set_header("X-Cache", "MISS");
        } else {
            res.status = 500;
            res.set_content("Internal Server Error: Cannot read tile", "text/plain");
        }
    });

    std::cout << "Storage Node booting up on port " << port << " (LRU Cache: 64 MB)\n";
    std::cout << "Serving dataset from: " << base_dir << "\n";

    svr.listen("0.0.0.0", port);
    return 0;
}