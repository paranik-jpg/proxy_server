# Multi-Threaded C++ HTTP Proxy Server

A high-performance, concurrent HTTP proxy server built from scratch in C++20. Designed to handle multiple incoming client requests while maintaining a flat memory footprint and ensuring graceful system termination. This server utilizes a Fixed-Capacity Thread Pool architecture to ensure predictable resource usage.

## Features
- **Concurrent Request Handling**: Utilizes a custom fixed-capacity Thread Pool to serve multiple clients efficiently, preventing resource exhaustion.
- **Thread Safety**: Implements `std::mutex` and `lock_guard` for synchronized console logging and shared thread-tracking management.
- **Robust Resource Management**: Uses RAII principles and manual POSIX socket management for efficient memory and file descriptor handling.
- **Graceful Shutdown**: Includes a custom `SIGINT` (Ctrl+C) signal handler to ensure all master sockets are closed properly, preventing port-binding deadlocks.
- **Containerized Deployment**: Multi-stage `Dockerfile` provided for seamless environment isolation and deployment.

## Technical Architecture
- **Networking**: Built using raw POSIX socket APIs (`socket`, `bind`, `listen`, `accept`, `recv`, `send`).
- **Concurrency**: Implements a custom ThreadPool using `std::condition_variable` and `std::mutex` for thread-safe task queuing.
- **Parsing**: Custom HTTP request parser for URL and Host extraction.
- **DNS**: Asynchronous lookups using `getaddrinfo`.

## Concurrency Model
- **Task Queuing**: Incoming client connections are wrapped as tasks and enqueued in a thread-safe `std::queue`.
- **Worker Threads**: A predefined pool of worker threads consumes these tasks using a `std::condition_variable`, eliminating the overhead and potential memory exhaustion of spawning threads per connection.
- **Fault Tolerance**: Each worker thread implements a socket-level timeout (`SO_RCVTIMEO`), ensuring that idle or dead connections do not block the system's throughput.

### Challenges & Solutions
- **Challenge**: Handling browser disconnections during data streaming.
- **Solution** : Implemented robust error checking on `recv()` return values and verified descriptor closing to prevent socket leaks.

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