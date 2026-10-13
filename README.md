# High-Performance C++17 Multithreaded HTTP Proxy Server

A hardened, high-throughput forward HTTP proxy server built from scratch in modern C++17 and Linux POSIX system calls. Engineered for low-level systems reliability, featuring a bounded producer-consumer thread pool with backpressure rejection, non-blocking signal handling, reliable stream socket transmission, and memory/file-descriptor leak validation using AddressSanitizer and Valgrind.

---

## Systems Architecture

```text
               ┌────────────────────────────────────────────────────────┐
               │                      ProxyServer                       │
               │                                                        │
Incoming       │  [ accept() ]                                          │
Clients ───────┼────────┬───────────────────────────────────────────────┤
               │        ▼                                               │
               │  ThreadPool Queue (Capacity: 1,000, 100ms Timeout)     │
               │        │                                               │
               │  [Backpressure Drop on Saturation]                     │
               │        │                                               │
               │        ▼                                               │
               │  Worker Threads (hardware_concurrency)                 │
               │        │                                               │
               │        ├── 1. Chunked recv() until \r\n\r\n (<= 8 KB)  │
               │        ├── 2. HTTP Header & Port Parsing               │
               │        ├── 3. POSIX getaddrinfo() DNS Resolution       │
               │        ├── 4. Non-blocking/Timeout connect()           │
               │        ├── 5. Reliable sendAll() (MSG_NOSIGNAL)        │
               │        └── 6. Bidirectional Streaming Pass-Through     │
               └──────────────────────────┬─────────────────────────────┘
                                          │
                                          ▼
                                   Upstream Web Server
```

---

## Core Engineering Invariants

### 1. Robust Concurrency & Backpressure

- **Fixed Worker Sizing:** Thread pool sized to `std::thread::hardware_concurrency()` to reduce unnecessary context switching and resource contention.
- **Bounded Task Queue:** Limits the internal task queue to 1,000 tasks to prevent unbounded queue growth during connection bursts.
- **Graceful Backpressure:** Uses a 100 ms condition-variable timeout on task submission; saturated requests are rejected and client file descriptors are closed cleanly.
- **Failure Domain Isolation:** Dispatched worker tasks execute inside `try/catch` boundaries to prevent application exceptions from terminating worker threads.

### 2. POSIX Sockets & Streaming Invariants

- **Reliable Transmission (`sendAll`):** Loops over `send()` to handle partial writes and retries interrupted system calls (`EINTR`).
- **Signal Safety:** Uses `MSG_NOSIGNAL` to suppress `SIGPIPE` on supported Linux systems, allowing socket errors such as `EPIPE` to be handled by the application.
- **Request Header Limits:** Aggregates incoming bytes across packet boundaries until detecting the `\r\n\r\n` delimiter, with an 8 KB header-size ceiling.
- **HTTP Parsing:** Supports case-insensitive header names, origin-form extraction, IPv6 authority bracket handling (`[::1]:port`), and port parsing.

### 3. Diagnostics & Memory Validation

- **Memory Integrity:** Uses AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) to detect memory errors and undefined behavior.
- **Descriptor Lifecycle:** Uses Valgrind Memcheck with `--track-fds=yes` to inspect heap allocations and file-descriptor lifecycle.

---

## Empirical Performance Benchmarks

Benchmarked using `wrk` on Linux x86-64 with `-O3 -DENABLE_INFO_LOGGING=0`.

| Scenario | Concurrency | Threads | Throughput | Avg Latency | p50 Latency | p90 Latency | p99 Latency | Max Latency | Completed Requests |
|:--|--:|--:|--:|--:|--:|--:|--:|--:|--:|
| **Keep-Alive Baseline** | 50 | 4 | 176.20 req/s | 274.12 ms | 237.56 ms | 396.73 ms | 813.48 ms | 910.68 ms | 3,543 |
| **Queue Saturation** | 500 | 8 | 141.30 req/s | 1.07 s | 1.02 s | 1.84 s | 1.98 s | 2.00 s | 4,254 |
| **Connection Churn** | 200 | 8 | 105.33 req/s | 1.19 s | 1.15 s | 1.73 s | 1.99 s | 2.00 s | 3,171 |

Full analysis, diagnostic traces, and socket lifecycle metrics are documented in [`docs/BENCHMARK.md`](docs/BENCHMARK.md).

---

## Project Structure

```text
proxy_server/
├── docs/
│   ├── BENCHMARK.md          # wrk latency profiles and Valgrind audit logs
│   ├── CONCURRENCY.md        # Thread pool synchronization and backpressure
│   ├── NETWORK_INTERNALS.md  # Socket options, framing, and sendAll semantics
│   └── BUILD_AND_LOGGING.md  # Build targets, sanitizers, and logging policy
├── include/
│   ├── HttpParser.h          # HTTP header and URI parsing interfaces
│   ├── Logger.h              # Thread-safe logging
│   ├── ProxyServer.h         # Server lifecycle and client socket orchestration
│   ├── SignalHandler.h       # POSIX signal registration
│   └── ThreadPool.h          # Bounded producer-consumer thread pool
├── src/
│   ├── HttpParser.cpp
│   ├── Logger.cpp
│   ├── ProxyServer.cpp
│   ├── SignalHandler.cpp
│   ├── ThreadPool.cpp
│   └── main.cpp              # CLI port parser and server entry point
├── Dockerfile
├── docker-compose.yml
├── CMakeLists.txt
└── README.md
```

---

## Build & Diagnostics

### 1. Production / High-Throughput Build

Optimized for performance benchmarking:

```bash
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-DENABLE_INFO_LOGGING=0 -O3"

cmake --build build -j"$(nproc)"

./build/ProxyServer 8888
```

### 2. Sanitizer Debug Build (ASan + UBSan)

Instrumented with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -B build_asan -S . \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_SANITIZERS=ON

cmake --build build_asan -j"$(nproc)"

ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
  ./build_asan/ProxyServer 8888
```

### 3. Valgrind File-Descriptor & Memory Audit

```bash
valgrind --tool=memcheck \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-fds=yes \
  ./build/ProxyServer 8888
```

---

## Containerized Deployment

Build and start the application using Docker Compose:

```bash
docker compose up --build
```

The server is intended to listen on port `8888` for this configuration. Ensure the Dockerfile, Compose port mapping, and application listening port agree.

---

## Design Trade-offs & Current Limitations

This is a lightweight HTTP proxy prototype focused on socket programming,
request parsing, upstream connections, and concurrent client handling. The
current implementation does not provide a complete HTTP forwarding relay.

### Current limitations

- **HTTP request bodies:** Complete request-body relaying is out of scope for
  the current prototype. The worker reads the request headers, but does not
  reliably continue streaming subsequent body bytes to the upstream server.
  POST/PUT uploads and chunked request bodies may therefore be incomplete or
  fail.
- **HTTPS `CONNECT`:** Full HTTPS tunneling is out of scope. The proxy does not
  establish a working `200 Connection Established` tunnel and relay opaque TLS
  bytes bidirectionally. Parsing an HTTPS destination is not equivalent to
  implementing a CONNECT tunnel.
- **Production readiness:** Do not treat this prototype as a production-ready
  or unrestricted public forward proxy. Destination restrictions, access
  controls, correct HTTP framing, and broader integration tests remain
  important hardening work.

### Design trade-offs

The implementation favors a small, understandable networking pipeline and a
bounded worker pool over full HTTP protocol support. This makes the project
useful for learning TCP sockets, parsing, concurrency, and resource management,
while keeping unsupported behavior explicit.

## Future Roadmap

- [ ] HTTPS `CONNECT` tunneling with TLS pass-through
- [ ] Zero-copy buffer forwarding using Linux `splice(2)`
- [ ] Epoll-based event loop using `epoll_wait()` for comparison with the thread-pool architecture
- [ ] LRU in-memory HTTP response caching

---

## License

Open-source under the MIT License.
