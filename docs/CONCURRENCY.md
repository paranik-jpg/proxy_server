## Bounded Task Queue and Backpressure

### Queue Capacity
The thread pool uses a bounded task queue with a maximum capacity of **1,000 pending tasks**. This limit bounds the number of queued tasks, preventing unlimited queue growth under overload.

### Enqueue Timeout
When the queue is full, an enqueue operation waits for up to **100 ms** for queue capacity to become available. If capacity becomes available before the timeout expires, the task is enqueued. Otherwise, the task is rejected.

### Overload Handling
If enqueueing fails because the queue remains full until the timeout expires, the server rejects the connection task and returns **HTTP 503 Service Unavailable**, where an HTTP response can still be sent.

Rejected tasks must not be silently discarded. The caller is responsible for handling the rejection and cleaning up any resources, including closing the client socket when ownership has not been transferred to the task.

### Shutdown Behavior
When the thread pool begins shutting down, new tasks are rejected. Producers waiting for queue capacity are notified and must stop waiting. Workers drain already-queued tasks before exiting.

### Resource Limits
The queue capacity bounds pending tasks, not total memory usage or active connections. Additional limits on concurrent connections, request duration, and upstream I/O may be required to protect the server under sustained load.
