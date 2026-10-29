#include "../../include/httplib.h"
#include "../hash_ring/hash_ring.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

using namespace fashion::storage;

// Read a node address from the environment (set by docker-compose), else use the local default
static std::string env_or(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return (value && *value) ? value : fallback;
}

int main() {
    // 1. Initialize the Hash Ring
    HashRing ring(100);

    // 2. Register our physical storage nodes
    ring.add_node({"node-A", env_or("NODE_A_ADDR", "localhost:8081")});
    ring.add_node({"node-B", env_or("NODE_B_ADDR", "localhost:8082")});
    ring.add_node({"node-C", env_or("NODE_C_ADDR", "localhost:8083")});

    httplib::Server gateway;

    // Allow the Next.js dashboard (localhost:3000) to load tiles cross-origin.
    // Default headers are attached to every response, including errors.
    gateway.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Methods", "GET, OPTIONS"},
    });

    // Handle CORS preflight requests
    gateway.Options(R"(/tiles/(.*))", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // 3. Listen for tile requests
    gateway.Get(R"(/tiles/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string tile_path = req.matches[1];
        
        // Use the tile path (e.g., "12/5_5.png") as our consistent hashing key
        auto target_node = ring.get_node(tile_path);
        
        if (!target_node) {
            res.status = 500;
            res.set_content("Cluster is down: No storage nodes available", "text/plain");
            return;
        }

        // 4. Proxy the request: Fetch the tile from the assigned backend node
        httplib::Client client(target_node->address);
        
        // Set a brief timeout for ULL (Ultra-Low Latency) environments
        client.set_connection_timeout(0, 50000); // 50ms
        client.set_read_timeout(0, 200000);      // 200ms
        
        auto node_res = client.Get("/tiles/" + tile_path);

        if (node_res && node_res->status == 200) {
            res.set_content(node_res->body, node_res->get_header_value("Content-Type"));
            res.set_header("X-Served-By-Node", target_node->id);
            if (node_res->has_header("X-Cache")) {
                res.set_header("X-Cache", node_res->get_header_value("X-Cache"));
            }
        } else if (node_res) {
            res.status = 404;
            res.set_content("Tile missing on backend " + target_node->id, "text/plain");
        } else {
            // Node didn't answer at all (not running, or timed out)
            res.status = 502;
            res.set_content("Backend " + target_node->id + " unreachable at " + target_node->address, "text/plain");
        }
    });

    std::cout << "Edge Gateway booting up...\n";
    std::cout << "Listening on http://localhost:8080\n";
    if (!gateway.listen("0.0.0.0", 8080)) {
        std::cerr << "Failed to listen on port 8080 (already in use?)\n";
        return 1;
    }

    return 0;
}