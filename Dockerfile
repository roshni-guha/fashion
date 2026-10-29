FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build the C++ binaries
RUN cmake -S . -B build && cmake --build build

# Expose ports for the nodes and gateway
EXPOSE 8080 8081 8082 8083

# Default command (can be overridden by docker-compose)
CMD ["./build/edge_router"]