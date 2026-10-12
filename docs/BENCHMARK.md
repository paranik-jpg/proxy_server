# Empirical Load & Performance Benchmarks

## 1. Test Harness & Environment
* **Platform:** Linux (x86_64, POSIX sockets)
* **Optimization Profile:** `-O3 -DENABLE_INFO_LOGGING=0` (GCC 15.2.0)
* **Load Generator:** `wrk` (Multi-threaded HTTP benchmarking tool)
* **Upstream Target:** Local HTTP mock endpoint (`127.0.0.1:9000`)

---

## 2. Empirical Performance Matrix

|         Scenario        | Concurrency | Threads |  Throughput  | Avg Latency | p50 Latency | p90 Latency | p99 Latency | Max Latency | Completed Reqs |
| :---------------------- | :---------- | :------ | :----------- | :---------- | :---------- | :---------- | :---------- | :---------- | :------------- |
| **Keep-Alive Baseline** |   50 conns  |    4    | 176.20 req/s |  274.12 ms  |  237.56 ms  |  396.73 ms  |  813.48 ms  |  910.68 ms  |     3,543      |
|  **Queue Saturation**   |  500 conns  |    8    | 141.30 req/s |     1.07 s  |     1.02 s  |     1.84 s  |     1.98 s  |     2.00 s  |     4,254      |
|  **Connection Churn**   |  200 conns  |    8    | 105.33 req/s |     1.19 s  |     1.15 s  |     1.73 s  |     1.99 s  |     2.00 s  |     3,171      |

---

## 3. Bottleneck & System Diagnostics
* **Upstream Serialization:** Upstream latency ceilings were determined by single-threaded upstream mock serialization rather than proxy thread contention.
* **TCP Reset & Broken Pipe Handling:** `wrk` client-side connection teardown (`RST`) triggers `EPIPE` (`errno = 32`). The proxy's `sendAll()` handles this cleanly via `MSG_NOSIGNAL` without raising unhandled `SIGPIPE` signals.

---

## 4. Memory & Descriptor Integrity Validation
* **AddressSanitizer (ASan) & UBSan:** Verified 50 continuous synthetic requests with zero buffer overflows, use-after-free, or alignment faults.
* **Valgrind Memcheck:**
  * Definitely Lost: `0 bytes in 0 blocks`
  * Indirectly Lost: `0 bytes in 0 blocks`
  * Open Sockets / File Descriptors at Exit: Verified clean release of worker sockets, returning to standard streams (`0`, `1`, `2`).
