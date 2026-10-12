# --- Stage 1: Build Environment ---
    FROM ubuntu:24.04 AS builder

    # Install build chain and CMake
    RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ \
        cmake \
        make \
        && rm -rf /var/lib/apt/lists/*

    WORKDIR /app

    # Copy project files
    COPY . .

    # Build release binary with logging gate
    RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DENABLE_INFO_LOGGING=0 -O3" && \
        cmake --build build -j$(nproc)

    # --- Stage 2: Runtime Environment ---
    FROM ubuntu:24.04

    WORKDIR /app

    # Copy compiled executable from builder stage
    COPY --from=builder /app/build/ProxyServer ./proxy_server

    # Expose internal listening port
    EXPOSE 8888

    # Run with explicit port argument
    CMD ["./proxy_server", "8888"]
