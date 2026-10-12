# Concurrency & Thread Synchronization

## 1. Thread Pool Architecture
* **Core Primitives:** Implemented using `std::vector<std::thread>`, `std::queue<std::function<void()>>`, `std::mutex`, and `std::condition_variable`.
* **Worker Sizing:** Initialized using `std::thread::hardware_concurrency()` to avoid context-switching overhead on the host CPU.

---

## 2. Bounded Queue & Backpressure Mechanics
* **Capacity Limit:** Queue capacity capped at 1,000 tasks.
* **Backpressure Policy:** Submissions into a saturated queue wait for slot availability via a timed predicate (`std::chrono::milliseconds(100)`). If saturation persists, `enqueue()` returns `false`, causing `ProxyServer` to drop the client descriptor to safeguard process memory.

---

## 3. Failure Domain Isolation
* **Task Boundary Protection:** Every dispatched task in worker threads is isolated inside a `try / catch (...)` block to prevent exceptions from bubbling up and invoking `std::terminate()`.
* **Teardown Synchronization:** Destructor sets an atomic stop flag, notifies all waiting worker threads via `notify_all()`, and executes `worker.join()` across every thread.
