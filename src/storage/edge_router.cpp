#include "../../include/httplib.h"
#include "../hash_ring/hash_ring.hpp"
#include <iostream>
#include <string>

using namespace fashion::storage;

int main() {
    // 1. Initialize the Hash Ring
    HashRing ring(100);

    // 2. Register our physical storage nodes
    ring.add_node({"node-A", "localhost:8081"});
    ring.add_node({"node-B", "localhost:8082"});
    ring.add_node({"node-C", "localhost:8083"});

    httplib::Server gateway;

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
            // Forward the image back to the client
            res.set_content(node_res->body, node_res->get_header_value("Content-Type"));
            
            // Inject a custom header so we can visually prove the routing worked
            res.set_header("X-Served-By-Node", target_node->id);
        } else {
            res.status = 404;
            res.set_content("Tile missing on backend " + target_node->id, "text/plain");
        }
    });

    std::cout << "Edge Gateway booting up...\n";
    std::cout << "Listening on http://localhost:8080\n";
    gateway.listen("0.0.0.0", 8080);

    return 0;
}