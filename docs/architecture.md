# CodonWeb Architecture & Internals

CodonWeb is an ahead-of-time (AOT) compiled web framework built on top of [Codon](https://github.com/exaloop/codon), an LLVM-based Python compiler. It is designed to combine the developer ergonomics and expressive syntax of modern Python web frameworks (such as FastAPI) with the raw execution speed, low memory footprint, and concurrency capabilities of C/C++ and Rust.

---

## Architecture Overview

```
                      +----------------------------------------+
                      |         HTTP / WebSocket Client        |
                      +----------------------------------------+
                                          |
                                   [TCP Socket]
                                          v
                      +----------------------------------------+
                      |       Linux Native Epoll Reactor       |
                      |   (Non-blocking I/O Event Dispatcher)  |
                      +----------------------------------------+
                                          |
                        [Zero-Copy Slicing: ByteSlice]
                                          v
                      +----------------------------------------+
                      |        HTTP 1.1 Fast Parser            |
                      | (RFC 7230 Chunked / Keep-Alive)       |
                      +----------------------------------------+
                                          |
                                    [Request Context]
                                          v
                      +----------------------------------------+
                      |        Middleware Onion Pipeline       |
                      |  - Recovery / Security Headers         |
                      |  - CORS / Trusted Hosts / Rate Limiter |
                      |  - RequestId / Timing / Sessions       |
                      +----------------------------------------+
                                          |
                                          v
                      +----------------------------------------+
                      |          Radix Tree Router             |
                      |    (Sub-microsecond Path Lookup)       |
                      +----------------------------------------+
                                          |
                        +-----------------+-----------------+
                        |                                   |
                        v                                   v
             [Dependency Resolution]               [Route Handler]
             - FastAPI-style Depends                - Sync Route
             - Request-level Cache                  - Async Route Coroutine
             - Parameter Binder & Schema            - WebSocket Handler
                        |                                   |
                        +-----------------+-----------------+
                                          |
                                          v
                      +----------------------------------------+
                      |   Native FastJson Engine (yyjson C-ABI)|
                      |    (SIMD Serialization/Deserialization)|
                      +----------------------------------------+
                                          |
                                          v
                      +----------------------------------------+
                      |      Async MySQL Pool / SQLite / SSE   |
                      +----------------------------------------+
                                          |
                                   [Response Slices]
                                          v
                      +----------------------------------------+
                      |             Socket Flush               |
                      +----------------------------------------+
```

---

## 1. Zero-Copy Memory Model (`ByteSlice`)

Traditional Python web frameworks allocate multiple intermediate string and byte objects when reading HTTP headers, query strings, and cookies. Under high concurrency, this causes significant Garbage Collector (GC) pressure and CPU cache misses.

CodonWeb employs `ByteSlice`:
- Operates directly over raw memory buffers allocated for incoming TCP sockets.
- Slicing a buffer creates a pointer window `(ptr, len)` rather than copying memory blocks.
- Conversion to typed native structures (e.g., integers, booleans, or specific string tokens) only occurs when requested by route handlers or parameter binders.

---

## 2. Sub-Microsecond Radix Tree Routing

URL routing uses a compressed prefix tree (Radix Tree / Patricia Trie):
- Lookup time complexity is $O(k)$, where $k$ is the length of the URI path string, completely independent of the total number of registered routes ($N$).
- Dynamic path parameters (`/users/{id:[0-9]+}/posts/{slug}`) are extracted during tree descent without auxiliary regular expression engine overhead.
- Supports route grouping (`RouterGroup`), nested prefixes, and per-group middleware inheritance.

---

## 3. FastJson Native C-ABI Engine

JSON serialization and deserialization are often the primary performance bottleneck in modern REST and microservice workloads. CodonWeb integrates with [yyjson](https://github.com/ibireme/yyjson), one of the fastest ANSI C JSON libraries in the world:

- **Direct C-ABI Linking**: CodonWeb interfaces with `libcfastjson.a` via native C calling conventions without Python CPython/PyObject wrapping overhead.
- **SIMD Accelerations**: Leverages AVX2, NEON, or SSE4.2 instructions depending on CPU target.
- **Zero-Copy String Extraction**: String values in JSON payloads can be read directly as string slices without reallocating heap memory.
- **Performance**: Delivers 5x to 14x throughput improvements compared to standard JSON libraries across payloads from 400 bytes to multi-megabyte datasets.

---

## 4. Onion Middleware Pipeline

CodonWeb implements a bidirectional onion-style middleware architecture:
```
Request ---> [ Middleware 1 ] ---> [ Middleware 2 ] ---> [ Route Handler ]
Response <--- [ Middleware 1 ] <--- [ Middleware 2 ] <--- [ Return Value ]
```

Built-in production middlewares include:
- `RecoveryMiddleware`: Catches unexpected panics/exceptions and produces RFC 7807 error responses.
- `CORSMiddleware`: Full CORS protocol handling with origin matching, headers, credentials, and preflight `OPTIONS` caching.
- `SecurityHeadersMiddleware`: Injects standard headers (`X-Content-Type-Options`, `X-Frame-Options`, `Strict-Transport-Security`, `Content-Security-Policy`).
- `RequestIdMiddleware`: Generates or propagates distributed tracing IDs (`X-Request-ID`).
- `RateLimiterMiddleware`: Token-bucket / sliding window rate limiting.
- `TimingMiddleware`: Measures server processing latency and injects `X-Process-Time`.
- `SignedCookieSessionMiddleware`: Tamper-proof HMAC-SHA256 signed cookie sessions.
- `HTTPSRedirectMiddleware`: Enforces TLS by redirecting HTTP traffic to HTTPS with 301/307.
- `TrustedHostMiddleware`: Guards against HTTP Host header poisoning attacks.

---

## 5. Dependency Injection Engine

Inspired by FastAPI's `Depends`:
- Route handlers can declare dependencies as function arguments.
- The engine analyzes dependency graphs and executes dependencies before invoking the route handler.
- **Request-Scoped Caching**: Dependencies that appear multiple times in a single request DAG are executed only once and cached in the request context.
- **Testing Overrides**: `app.dependency_overrides` allows swapping production dependencies (e.g., live databases, external payment gateways) with in-memory mocks during automated tests.

---

## 6. Native Async Epoll Reactor & Coroutines

Unlike interpreter-based event loops (such as Python's standard `asyncio`), CodonWeb compiles coroutines into native state machines:
- Direct system calls to Linux `epoll_create1`, `epoll_ctl`, and `epoll_wait`.
- Non-blocking socket read/write operations integrated with event reactor queues.
- Asynchronous MySQL driver and connection pooling (`AsyncMySQLPool`) execute non-blocking queries and connection reuse without thread context switching overhead.

---

## 7. In-Memory `TestClient`

For testing, CodonWeb provides an in-memory test harness:
- Directly dispatches synthetic `Request` objects into the middleware and routing pipeline.
- Bypasses TCP network socket binding and OS network stack latency.
- Executes full integration suites in milliseconds.
