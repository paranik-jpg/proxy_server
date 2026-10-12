# Network & Socket Internals

## 1. Socket Lifecycle Architecture
* **Listening Socket Setup:** Uses `AF_INET`, `SOCK_STREAM` with `SO_REUSEADDR` and `SO_REUSEPORT` flags enabled to facilitate rapid process restart and avoid `EADDRINUSE` failures.
* **Backlog Depth:** Tuned `listen(server_fd, 128)` to absorb concurrent connection bursts during connection churn.
* **Timeout Guards:** Sets `SO_RCVTIMEO` (5 seconds) on incoming client descriptors and upstream remote sockets to prevent inactive connections from holding worker threads indefinitely.

---

## 2. Stream Framing & Delimitation Invariants
* **Partial Read Accumulation:** Because TCP is a continuous byte stream without packet-boundary guarantees, requests are aggregated through an iterative buffer loop until encountering the HTTP delimiter `\r\n\r\n`.
* **Slowloris DoS Defense:** Accumulation enforces an explicit size ceiling (`MAX_HEADER_SIZE = 8192`). Connections exceeding 8 KB before delimiter termination are dropped immediately.
* **Zero Truncation Outbound Relay:** The complete accumulated string buffer (`request.data()`, `request.size()`) is passed into `sendAll()`, preventing the loss of fragmented request headers.

---

## 3. Atomic Outbound Writes (`sendAll`)
* Iterative `send()` loop handles short writes under partial buffer availability.
* Interrupted system calls (`errno == EINTR`) automatically retry without terminating the connection.
* `MSG_NOSIGNAL` suppresses kernel `SIGPIPE` signals on disconnected peer sockets, translating resets into return errors (`EPIPE`).
