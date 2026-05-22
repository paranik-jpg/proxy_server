# Multi-Threaded C++ HTTP Proxy Server

A high-performance, concurrent HTTP proxy server built from scratch in C++20. Designed to handle multiple incoming client requests while maintaining a flat memory footprint and ensuring graceful system termination.

## Features
- **Concurrent Request Handling**: Utilizes `std::jthread` and a thread-pool-like model to serve multiple clients without blocking.
- **Thread Safety**: Implements `std::mutex` and `lock_guard` for synchronized console logging and shared thread-tracking management.
- **Robust Resource Management**: Uses RAII principles and manual POSIX socket management for efficient memory and file descriptor handling.
- **Graceful Shutdown**: Includes a custom `SIGINT` (Ctrl+C) signal handler to ensure all master sockets are closed properly, preventing port-binding deadlocks.
- **Containerized Deployment**: Multi-stage `Dockerfile` provided for seamless environment isolation and deployment.

## Technical Architecture
- **Networking**: Built using raw POSIX socket APIs (`socket`, `bind`, `listen`, `accept`, `recv`, `send`).
- **Concurrency**: C++20 `std::jthread` for lifecycle management.
- **Parsing**: Custom HTTP request parser for URL and Host extraction.
- **DNS**: Asynchronous lookups using `getaddrinfo`.

## Getting Started

### Prerequisites
- Docker & Docker Compose installed.

### Running with Docker
1. Clone the repository: `git clone <your-repo-url>`
2. Build and launch:
   ```bash
   docker compose up --build -d
3. Check logs:
   ```bash
   docker logs cpp_proxy_engine

## License

- This project is open-source and intended for educational use.