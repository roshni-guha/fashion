#include "httplib.h"
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>

// Run: ./node_server 8081 ../ingest/dataset/fashion_item_files
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <dataset_directory>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string base_dir = argv[2];
    const std::filesystem::path root = std::filesystem::canonical(base_dir);

    httplib::Server svr;

    // Route: GET /tiles/{z}/{x}_{y}.png
    svr.Get(R"(/tiles/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string tile_path = req.matches[1]; // Extracts "10/0_0.png"
        std::filesystem::path full_path = std::filesystem::weakly_canonical(std::filesystem::path(base_dir) / tile_path);

        // Reject paths that escape the dataset directory (e.g. "../../etc/passwd")
        auto [root_end, _] = std::mismatch(root.begin(), root.end(), full_path.begin(), full_path.end());
        if (root_end != root.end()) {
            res.status = 403;
            res.set_content("Forbidden", "text/plain");
            return;
        }

        if (!std::filesystem::is_regular_file(full_path)) {
            res.status = 404;
            res.set_content("Tile not found", "text/plain");
            return;
        }

        // Read file into memory and serve
        std::ifstream file(full_path, std::ios::binary);
        if (file) {
            std::string content((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
            res.set_content(content, "image/png");
        } else {
            res.status = 500;
            res.set_content("Internal Server Error: Cannot read tile", "text/plain");
        }
    });

    std::cout << "Storage Node booting up...\n";
    std::cout << "Serving dataset from: " << base_dir << "\n";
    std::cout << "Listening on http://localhost:" << port << "\n";

    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "Failed to listen on port " << port << "\n";
        return 1;
    }

    return 0;
}