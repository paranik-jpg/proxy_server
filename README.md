# ProxyServer

A high-performance, multi-threaded HTTP proxy server built from scratch in C++17. The project demonstrates low-level network programming using POSIX sockets, concurrent request processing with a custom thread pool, modular software architecture, and efficient HTTP request forwarding.

## Features

* **Multi-Threaded Request Processing**: Handles multiple client connections concurrently using a custom fixed-size thread pool.
* **HTTP Request Parsing**: Extracts request URLs, target hosts, and ports through a dedicated HTTP parser.
* **DNS Resolution**: Resolves domain names using the POSIX `getaddrinfo()` API before establishing outbound connections.
* **Request Forwarding**: Transparently forwards HTTP requests to remote servers and relays responses back to clients.
* **Graceful Shutdown**: Handles `SIGINT` (Ctrl+C) to close the listening socket cleanly and prevent port binding issues.
* **Thread-Safe Logging**: Centralized logging module for synchronized console output across worker threads.
* **Socket Timeouts**: Applies configurable receive timeouts to both client and upstream sockets using `SO_RCVTIMEO`, preventing stalled connections from blocking worker threads.
* **Modular Architecture**: Organized into independent components for networking, parsing, threading, logging, and signal handling.
* **Docker Support**: Includes Docker and Docker Compose configurations for reproducible deployment.
* **Connection Relay**: Streams responses from remote servers back to clients in fixed-size chunks without buffering the entire response in memory.

---

## Performance Characteristics

- Fixed-size worker thread pool
- O(1) task scheduling using a synchronized task queue
- Streaming response relay with constant memory overhead
- Thread-safe request processing
- DNS resolution using POSIX `getaddrinfo()`

## Project Structure

```
ProxyServer/
│
├── include/
│   ├── HttpParser.h
│   ├── Logger.h
│   ├── ProxyServer.h
│   ├── SignalHandler.h
│   └── ThreadPool.h
│
├── src/
│   ├── HttpParser.cpp
│   ├── Logger.cpp
│   ├── ProxyServer.cpp
│   ├── SignalHandler.cpp
│   ├── ThreadPool.cpp
│   └── main.cpp
│
├── tests/
├── Dockerfile
├── docker-compose.yml
├── CMakeLists.txt
└── README.md
```

---

## Technical Architecture

### Networking

* POSIX Socket API

  * `socket()`
  * `bind()`
  * `listen()`
  * `accept()`
  * `connect()`
  * `recv()`
  * `send()`
  * `close()`

### Concurrency

* Custom fixed-size Thread Pool
* `std::thread`
* `std::mutex`
* `std::condition_variable`
* Thread-safe task queue

### HTTP Processing

* Request line parsing
* Host header extraction
* Default and custom port detection
* URL extraction
* HTTP request forwarding

### DNS Resolution

* `getaddrinfo()`
* IPv4 support using `addrinfo`

---

## Concurrency Model

1. The proxy listens for incoming client connections.
2. Each accepted connection is submitted as a task to the Thread Pool.
3. Worker threads process requests independently.
4. The request is parsed to determine the destination host and port.
5. DNS resolution is performed.
6. A connection to the remote server is established.
7. The original request is forwarded.
8. The remote response is streamed back to the client.

This design avoids creating one thread per client, resulting in predictable memory usage and improved scalability.

---

## Engineering Highlights

### Custom Thread Pool

A producer-consumer architecture backed by a thread-safe task queue enables efficient request scheduling while minimizing thread creation overhead.

### Modular Design

The project follows the Single Responsibility Principle by separating concerns into dedicated modules:

* **ProxyServer** – Server lifecycle and client management
* **ThreadPool** – Concurrent task execution
* **HttpParser** – HTTP request parsing
* **Logger** – Thread-safe logging
* **SignalHandler** – Graceful process termination

### Robust Resource Management

* Automatic cleanup of socket descriptors
* Proper `addrinfo` resource deallocation
* Receive timeouts for client and upstream sockets
* Graceful shutdown via signal handling
* Error checking across networking operations

The current implementation focuses on correctness, modularity, and concurrent request handling while providing a solid foundation for advanced proxy features such as caching, HTTPS tunneling, and persistent connections.

---

## Build Instructions

### Prerequisites

* CMake 3.16+
* GCC / Clang with C++17 support
* Linux (POSIX socket APIs)

### Build

```bash
cmake -S . -B build
cmake --build build
./build/ProxyServer
```

### Run

```bash
./ProxyServer
```

The server listens on **port 8888** by default.

---

## Docker

Build and run using Docker Compose:

```bash
docker compose up --build
```

The proxy server will be available on **localhost:8888**.

---

## Future Improvements

* HTTPS `CONNECT` tunneling
* HTTP response caching (LRU)
* Configuration file support
* Access logging
* Blacklisting and filtering
* Load balancing
* Unit and integration tests
* IPv6 support
* HTTP/1.1 Keep-Alive support
* Partial send() handling
* HTTPS CONNECT tunneling
* Response compression

---

## Technologies Used

* C++17
* Linux System Programming
* POSIX Socket API
* CMake
* Docker
* STL
* Multithreading

---

## License

This project is open-source and intended for educational and learning purposes.
