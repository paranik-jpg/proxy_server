# --- Stage 1: Build Environment ---
FROM ubuntu:24.04 AS builder

# Install core compiler and build tools
RUN apt-get update && apt-get install -y \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the entire directory over to build
COPY . .

# Compile using C++20 and explicit thread links
RUN g++ -std=c++20 -pthread src/main.cpp -o proxy_server

# --- Stage 2: Runtime Environment ---
FROM ubuntu:24.04

WORKDIR /root/

#Pull ONLY the compiled binary file from the builder stage
COPY --from=builder /app/proxy_server .

# Expose port 8080 to matches your proxy setup
EXPOSE 8080

# Execute the engine
CMD ["./proxy_server"]