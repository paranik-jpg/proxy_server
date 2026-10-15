# Project Flowchart — `proxy_server`

These diagrams explain the **implemented control flow**.

## How to read thid
- **Rectangle**: operation / component.
- **Diamond**: decision.
- **Solid arrows**: normal control or data flow.
- The Mermaid diagrams render on GitHub and in VS Code with a Mermaid preview extension.
- Start with each project's **big picture**, then follow the request/transaction flow.

---

# `proxy_server`

## 1. Big-picture architecture

```mermaid
flowchart TB
    C[Browser / HTTP Client] -->|TCP connection| L[ProxyServer listener]
    L --> A[accept client socket]
    A --> Q{Thread-pool queue has space?}
    Q -->|Yes| W[Worker thread takes task]
    Q -->|No / timeout| X[Reject task and close client socket]
    W --> H[handleClient]
    H --> P[HttpParser: host, URI, port]
    P --> D[getaddrinfo: resolve destination]
    D --> S[Create outbound socket]
    S --> K[connect to upstream server]
    K --> R[sendAll: forward request]
    R --> B[Relay upstream response to client]
    B --> CL[Close client and upstream sockets]
    CL --> END([Request complete])
    E[Logger] -. diagnostics .-> L
    E -. diagnostics .-> H
    SH[SignalHandler: SIGINT] -. shutdown signal .-> L
    TP[ThreadPool: workers + bounded queue] --- Q
```

**Mental model:** the listener accepts connections; the pool decides which worker handles each client; a worker parses one HTTP request, connects upstream, forwards bytes, relays the response, and cleans up descriptors.

## 2. Process startup and listener lifecycle

```mermaid
flowchart TD
    START([main]) --> ARGS{Port argument supplied?}
    ARGS -->|No| DEFAULT[Use default port 8888]
    ARGS -->|Yes| PARSE[Parse integer port]
    PARSE --> VALID{Port is 1..65535 and fully parsed?}
    VALID -->|No| ERR[Print usage / invalid-port error]
    ERR --> EXIT1([Exit with failure])
    VALID -->|Yes| CUSTOM[Use supplied port]
    DEFAULT --> CONSTRUCT[Construct ProxyServer and ThreadPool]
    CUSTOM --> CONSTRUCT
    CONSTRUCT --> STARTSERVER[ProxyServer.start]
    STARTSERVER --> SOCKET[Create listening socket]
    SOCKET --> OPT[Configure socket options]
    OPT --> BIND[bind to configured port]
    BIND --> LISTEN[listen for incoming clients]
    LISTEN --> SIGNAL[Register SIGINT handler]
    SIGNAL --> ACCEPT[accept client connection]
    ACCEPT --> ENQUEUE[Submit handleClient task to pool]
    ENQUEUE --> MORE{Listener still running?}
    MORE -->|Yes| ACCEPT
    MORE -->|No / startup or accept failure| CLEAN[Close listening resources]
    CLEAN --> EXIT([Return from process])
    SHUT[Ctrl+C / SIGINT] --> HANDLER[SignalHandler closes listening socket and exits]
```

> The default port in this source snapshot is **8888**. The README's architecture diagram describes the general request pipeline; use the code for exact configuration details.

## 3. One client request: end-to-end path

```mermaid
flowchart TD
    START([Worker begins handleClient]) --> TIMEOUT[Set client receive timeout]
    TIMEOUT --> READ[recv client bytes into request buffer]
    READ --> HEADER{Header terminator CRLF CRLF found?}
    HEADER -->|No, more bytes needed| LIMIT{Header limit or read failure reached?}
    LIMIT -->|No| READ
    LIMIT -->|Yes| FAIL[Log / abandon malformed or incomplete request]
    HEADER -->|Yes| PARSE[Extract destination from request]
    PARSE --> DEST{Usable host and port?}
    DEST -->|No| FAIL
    DEST -->|Yes| DNS[getaddrinfo destination]
    DNS --> DNSOK{Resolution succeeded?}
    DNSOK -->|No| FAIL
    DNSOK -->|Yes| TRY[Try resolved address candidates]
    TRY --> CONNECT{Outbound connect succeeds?}
    CONNECT -->|No candidate works| FAIL
    CONNECT -->|Connected| SEND[sendAll forwards complete request]
    SEND --> SENDOK{Forwarding succeeded?}
    SENDOK -->|No| CLEAN[Close descriptors]
    SENDOK -->|Yes| RECV[recv upstream response]
    RECV --> DATA{Bytes received?}
    DATA -->|Yes| CLIENT[sendAll response bytes to client]
    CLIENT --> IOOK{I/O successful?}
    IOOK -->|Yes| RECV
    IOOK -->|No| CLEAN
    DATA -->|EOF / timeout / error| CLEAN
    FAIL --> CLEAN
    CLEAN --> END([Worker task ends])
```

**Key distinction:** `sendAll` handles partial writes and retries `EINTR`; it does not mean the entire HTTP protocol is parsed. The relay is a byte-stream forwarding loop after the request is sent.

## 4. Thread-pool queue and worker lifecycle

```mermaid
flowchart TD
    SUBMIT([Listener submits client task]) --> LOCK[Lock queue mutex]
    LOCK --> SPACE{Queue space available before timeout?}
    SPACE -->|No| REJECT[Return enqueue failure]
    REJECT --> CLOSE[Caller closes rejected client socket]
    SPACE -->|Yes| PUSH[Push task into queue]
    PUSH --> UNLOCK[Unlock and notify worker]
    UNLOCK --> WAIT[Worker waits on condition variable]
    WAIT --> READY{Stop requested and queue empty?}
    READY -->|Yes| STOP([Worker exits])
    READY -->|No| POP[Move front task out and pop queue]
    POP --> NOTIFY[Notify that queue space is available]
    NOTIFY --> RUN[Execute task outside queue lock]
    RUN --> EXCEPTION{Task throws?}
    EXCEPTION -->|Yes| LOG[Catch and report exception]
    EXCEPTION -->|No| WAIT
    LOG --> WAIT
```

**Concurrency idea:** the queue mutex protects queue state; workers release it before running client code, so a slow network request does not hold the queue lock.

## 5. Source-file responsibility map

```mermaid
flowchart LR
    MAIN[src/main.cpp<br/>CLI / port validation] --> SERVER[src/ProxyServer.cpp<br/>socket lifecycle + request handling]
    SERVER --> PARSER[src/HttpParser.cpp<br/>HTTP destination parsing]
    SERVER --> POOL[src/ThreadPool.cpp<br/>bounded task queue + workers]
    SERVER --> LOGGER[src/Logger.cpp<br/>serialized diagnostics]
    SERVER --> SIGNAL[src/SignalHandler.cpp<br/>SIGINT handling]
    HEADER[include/*.h<br/>interfaces and state] -. contracts .-> MAIN
    HEADER -. contracts .-> SERVER
```

---

# Suggested reading order

1. `src/main.cpp` — how the program starts and selects the port.
2. `src/ProxyServer.cpp` — listener lifecycle and `handleClient` (the central request path).
3. `src/HttpParser.cpp` — how destination host/port are extracted.
4. `src/ThreadPool.cpp` — how work is queued and executed.
5. `src/SignalHandler.cpp`, then `src/Logger.cpp` — shutdown and diagnostics.

## Important interpretation note
- These are **code-understanding flowcharts**, not a certification that every advertised property is fully correct under every failure mode.
