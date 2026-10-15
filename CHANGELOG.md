# Changelog

All notable changes to **CodonWeb** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.2.0] - 2026-10-15

### 🚀 Added
- **Code-First AOT Schema Engine (`src/schema/`)**:
  - Declarative `BaseModel` / `SchemaModel` with typed fields (`StrField`, `IntField`, `FloatField`, `BoolField`, `ListField`, `ModelField`).
  - Automated code generation pipeline via `make codegen` using AST extraction and modern Jinja2 templating.
  - Zero-reflection SIMD deserialization (`from_json_doc`), field constraint enforcement, and custom business validation hooks (`custom_validate`).
  - Structured, RFC 7807 compliant HTTP 422 Unprocessable Entity error payloads.
  - Double-Key Verification mechanism ensuring zero naming collisions with external ORM base classes.
- **Industrial ORM & SQL Expression Builder (`src/orm/`)**:
  - Multi-dialect support for MySQL (5.7+/8.0+) and PostgreSQL (12+).
  - Declarative `Model`, `ModelMeta`, and column types (`IntegerField`, `CharField`, `DateTimeField`, etc.).
  - SQLAlchemy-style fluent query builder: `select()`, `where()`, `order_by()`, `limit()`, `offset()`, `and_()`, `or_()`, `not_()`, `desc()`, `asc()`, and aggregations (`func.count`, `func.avg`).
  - Automatic multi-dialect SQL compilation (backtick identifiers with `?` vs double quotes with `$1, $2`).
  - `DatabaseEngine`, `Session`, and non-blocking `AsyncSession`.
- **Native PostgreSQL Drivers & Async Pool (`src/db/drivers/postgres/`)**:
  - High-performance C FFI bridge with PostgreSQL client library (`libpq`).
  - Non-blocking asynchronous connection (`AsyncPostgresConnection`) and connection pool (`AsyncPostgresPool`).
  - Full PEP 249 compliant error hierarchy and `Row` abstractions.
- **TaskQ Distributed Task Queue (`src/taskq/`)**:
  - Pure native Codon distributed background task queue with zero Python runtime dependency.
  - Redis broker (`RedisBroker`) and in-memory broker (`MemoryBroker`).
  - Synchronous Worker (`Worker`) for CPU-bound computations.
  - Epoll-driven `AsyncWorker` delivering up to 28x throughput speedup on concurrent I/O operations (web crawlers, webhooks).
  - Delayed task scheduling with Redis Sorted Sets (ETA), priority queues, exponential backoff retries, and Dead Letter Queues (DLQ).
  - Cooperative task cancellation with `CancellationToken`.
- **Native Redis Driver (`src/db/drivers/redis/`)**:
  - Native implementation of the Redis RESP protocol for fast caching and broker synchronization.
- **Unified Single Event Loop (USEL) & Kernel I/O**:
  - Unified reactor integrating socket I/O, min-heap timer queues, and coroutine resumption.
  - Linux `io_uring` asynchronous subsystem integration (`src/runtime/iouring.codon`, `src/native/c_iouring.c`).
  - PicoHTTPParser integration via C-ABI FFI for ultra-fast single-pass HTTP parsing.
  - RFC 7231 cached HTTP `Date` header updating once per second to eliminate repetitive `strftime` calls.
- **Security & Industrial Hardening**:
  - Per-IP rate limiting and sliding window token bucket.
  - Cryptographic HMAC-SHA256 signed cookie sessions.
  - Path traversal protection, HTTP request smuggling prevention, and CRLF injection guards.
- **Benchmarks & Tooling**:
  - Side-by-side comparative benchmark harness against Rust Actix-Web (`benchmarks/`).
  - Expanded test runner from 26 to 33 comprehensive unit and integration test suites (`tests/run_all.codon`).
- **Complete Documentation Overhaul**:
  - New comprehensive bilingual documentation in `README.md` and `README_zh.md`.
  - In-depth architectural guide (`docs/architecture.md`).
  - Performance benchmarks guide (`docs/benchmarks.md`).
  - AOT Schema guide (`docs/schema_guide.md`).
  - ORM & SQL Builder guide (`docs/orm_guide.md`).
  - Production deployment & Docker guide (`docs/deployment.md`).

### ⚡ Optimized
- Zero-copy request parameter binding utilizing `ByteSlice`.
- SIMD JSON parsing using `yyjson` reaching 2.4 - 3.1 GB/s throughput.
- Worker CPU affinity pinning (`pthread_setaffinity_np`) to maximize L1/L2 cache locality.

---

## [0.1.0] - 2026-09-21

### 🚀 Initial Release
- Initial release of CodonWeb high-performance compiled web framework.
- Core `App` engine with Radix tree router and sub-microsecond path lookup.
- 9 production onion middlewares: CORS, Recovery, RequestId, Timing, SecurityHeaders, RateLimiter, TrustedHost, HTTPSRedirect, Session.
- Native MySQL client with connection pools (`MySQLPool`, `AsyncMySQLPool`).
- RFC 6455 WebSocket protocol implementation (handshakes, framing, ping/pong).
- Server-Sent Events (SSE) and HTTP/1.1 chunked transfer streaming.
- Automated OpenAPI 3.0 generation with Swagger UI and ReDoc.
- In-memory `TestClient` for zero-socket integration testing.
- Initial native C `yyjson` FastJson acceleration library.
