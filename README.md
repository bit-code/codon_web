<p align="center">
  <h1 align="center">⚡ CodonWeb</h1>
  <p align="center">
    <strong>Ultra-High-Performance, LLVM AOT-Compiled Modern Web Framework with FastAPI-Parity Developer Experience</strong>
  </p>
  <p align="center">
    <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
    <a href="https://github.com/exaloop/codon"><img src="https://img.shields.io/badge/compiler-Codon%20LLVM-orange.svg" alt="Codon LLVM"></a>
    <a href="https://github.com/ibireme/yyjson"><img src="https://img.shields.io/badge/fast--json-yyjson%20C--ABI-brightgreen.svg" alt="yyjson"></a>
    <img src="https://img.shields.io/badge/throughput-400k%2B%20QPS-red.svg" alt="Throughput">
    <img src="https://img.shields.io/badge/routing-sub--microsecond-purple.svg" alt="Sub-Microsecond Routing">
  </p>
  <p align="center">
    <a href="README.md">English</a> • <a href="README_zh.md">简体中文</a>
  </p>
</p>

---

## 🚀 Overview

**CodonWeb** is a high-performance web framework designed for the [Codon](https://github.com/exaloop/codon) compiler. It compiles idiomatic, clean Python syntax directly into native machine code via **LLVM Ahead-of-Time (AOT)** compilation—completely eliminating the Python Global Interpreter Lock (GIL), bytecode interpreter latency, and dynamic dispatch overhead.

CodonWeb combines the **ergonomics and developer joy of FastAPI** (dependency injection, typed parameter binding, automated OpenAPI 3.0, Swagger UI, and typed exceptions) with the **raw performance and low memory footprint of Rust and C++** (epoll non-blocking event loop, zero-copy memory slices, and SIMD-accelerated JSON).

---

## ✨ Key Features

- 🏎️ **LLVM AOT Native Speed**: Native machine code execution; sub-millisecond latency and throughput exceeding **400,000+ QPS**.
- 🧩 **FastAPI-Parity Developer Experience**:
  - Declarative **Dependency Injection** (`Depends`) with request-level DAG resolution and testing overrides.
  - Multi-source **Parameter Binding** (`Path`, `Query`, `Header`, `Cookie`) with automatic typed validation and RFC-compliant `422 Unprocessable Entity` responses.
  - Built-in **OpenAPI 3.0** specification generation with interactive **Swagger UI** (`/docs`) and **ReDoc** (`/redoc`).
  - Structured **HTTPException** hierarchy (`BadRequestException`, `UnauthorizedException`, `NotFoundException`, etc.).
- ⚡ **Sub-Microsecond Radix Tree Router**: $O(k)$ path lookup independent of route count, with route groups, prefixes, and per-group middlewares.
- 🚀 **SIMD FastJson Engine**: Native C-ABI zero-copy parser powered by [yyjson](https://github.com/ibireme/yyjson), delivering **5x to 14x** speedups over standard JSON libraries and outperforming Python `orjson`.
- 🔄 **Native Non-Blocking Epoll Reactor**: Linux native epoll event loop for high-concurrency connection handling with low CPU overhead.
- 🛡️ **Production-Ready Onion Middleware**:
  - `CORSMiddleware`, `RateLimiterMiddleware`, `SecurityHeadersMiddleware`
  - `RequestIdMiddleware`, `TimingMiddleware`, `TrustedHostMiddleware`
  - `SignedCookieSessionMiddleware`, `HTTPSRedirectMiddleware`, `RecoveryMiddleware`
- 🗄️ **Full Database Connectivity**:
  - Native MySQL client with **sync and async connection pools** (`MySQLPool`, `AsyncMySQLPool`).
  - Embedded SQLite driver (`SQLiteConnection`).
- 💬 **Real-time Protocols & Streaming**:
  - Full **RFC 6455 WebSocket** implementation (handshake, text/binary frames, masking, ping/pong).
  - Server-Sent Events (**SSE**) and HTTP Chunked Transfer Streaming.
- 🧪 **In-Memory `TestClient`**: Lightning-fast end-to-end testing without binding physical TCP sockets.

---

## 📊 Performance Benchmarks

### 1. HTTP Server Throughput (wrk: 400 Concurrency, 12 Threads)

| Framework | Runtime | Requests / sec (QPS) | Latency (P50) | Memory Footprint (RSS) |
|---|---|---|---|---|
| **CodonWeb** | **Codon (LLVM AOT)** | **420,000+** | **0.78 ms** | **~18 MB** |
| Actix-web | Rust Native | 460,000 | 0.71 ms | ~25 MB |
| Gin | Go 1.22 | 260,000 | 1.42 ms | ~38 MB |
| Fastify | Node.js 20 | 85,000 | 4.20 ms | ~95 MB |
| FastAPI + Uvicorn | Python 3.12 | 18,500 | 21.60 ms | ~120 MB |

### 2. JSON Serialization & Deserialization Speed

| Payload Size | Codon Standard `json` | Python `orjson` | **Codon FastJson (yyjson)** | Acceleration Ratio |
|---|---|---|---|---|
| **0.4 KB (Web API Object)** | 520k ops/s | 2,630k ops/s | **6,670k ops/s** | **12.8x faster** |
| **92 KB (1,000 Items)** | 2.4k ops/s | 12.8k ops/s | **26.3k ops/s** | **10.8x faster** |
| **1.4 MB (15,000 Items)** | 160 ops/s | 847 ops/s | **1,818 ops/s** | **11.4x faster** |

*For detailed benchmarks and reproduction steps, see [docs/benchmarks.md](docs/benchmarks.md).*

---

## 📦 Getting Started

### Prerequisites
- **OS**: Linux (Ubuntu 20.04+ recommended) or WSL2 on Windows, or macOS.
- **Build Tools**: `gcc` (or `clang`), `make`.
- **Codon Compiler**: [Install Codon](https://github.com/exaloop/codon):
  ```bash
  curl -fsSL https://exaloop.io/install.sh | bash
  echo 'export PATH="$HOME/.codon/bin:$PATH"' >> ~/.bashrc
  source ~/.bashrc
  ```

### Build Native Extensions
Clone the repository and compile the native C `yyjson` acceleration library:
```bash
git clone https://github.com/<your-username>/codon_web.git
cd codon_web

# Compile libcfastjson.a
make native
```

---

## 💡 Quick Example

Create `main.codon`:

```python
from codon_web import App, Request, Response, CORSMiddleware, TimingMiddleware

# 1. Initialize application with OpenAPI documentation
app = App(
    host="0.0.0.0",
    port=8000,
    title="CodonWeb API",
    version="1.0.0",
    docs_url="/docs"
)

# 2. Add production middlewares
app.use(CORSMiddleware(allow_origin="*"))
app.use(TimingMiddleware())

# 3. Define routes
@app.get("/")
def home(req: Request) -> Response:
    return Response.json('{"message": "Hello from compiled CodonWeb!"}')

@app.get("/users/{user_id}")
def get_user(req: Request) -> Response:
    user_id = req.param("user_id")
    return Response.json('{"id": ' + user_id + ', "status": "active"}')

if __name__ == "__main__":
    app.run()
```

### Compile and Run:
```bash
# Compile Ahead-of-Time to an optimized native binary
CODON_PATH=. codon build -release --linker-flags 'src/native/libcfastjson.a' -o app main.codon

# Execute native binary
./app
```
Open [http://localhost:8000/docs](http://localhost:8000/docs) in your browser to explore interactive Swagger UI documentation!

---

## 🛠️ Feature Walkthrough

### 1. Multi-Source Parameter Binding & Validation
Automatically bind and validate HTTP parameters from Query, Path, Header, or Cookie:
```python
from codon_web import Param, ParamsBinder, Request, Response, BadRequestException

item_params = [
    Param.path_int("item_id", description="Product ID"),
    Param.query_str("category", default_value="general"),
    Param.query_int("limit", default_value="20", min_val=1, max_val=100)
]
binder = ParamsBinder(item_params)

@app.get("/items/{item_id}")
def read_item(req: Request) -> Response:
    parsed, err = binder.bind(req)
    if not err.is_empty():
        return Response.json(err.to_json(), 422)
    
    item_id = parsed.get("item_id")
    category = parsed.get("category")
    return Response.json('{"item_id": ' + item_id + ', "category": "' + category + '"}')
```

### 2. FastAPI-Style Dependency Injection
```python
from codon_web import Request, Response, HTTPBearer, HTTPException

security = HTTPBearer()

def get_current_user(req: Request) -> str:
    token = security(req)
    if token != "secret-token":
        raise HTTPException(401, "Invalid or expired token")
    return "admin_user"

@app.get("/dashboard")
def dashboard(req: Request) -> Response:
    # Execute and cache dependency in request lifecycle
    user = get_current_user(req)
    return Response.json('{"authorized_user": "' + user + '"}')
```

### 3. High-Performance FastJson Parsing
```python
from codon_web import FastJson

# Zero-copy, SIMD-accelerated JSON deserialization
doc = FastJson.loads('{"name": "CodonWeb", "version": 1, "tags": ["fast", "compiled"]}')
print(doc["name"].as_str())     # "CodonWeb"
print(doc["version"].as_int())  # 1

# Instant serialization
output_str = doc.dumps()
doc.close()
```

### 4. Non-Blocking Async Database Queries
```python
from codon_web import AsyncMySQLPool, Request, Response

db_pool = AsyncMySQLPool(
    host="127.0.0.1", port=3306,
    user="root", password="password",
    db="production", pool_size=16
)

@app.get("/async/users")
async def get_users(req: Request) -> Response:
    conn = await db_pool.acquire()
    cursor = await conn.cursor()
    await cursor.execute("SELECT id, username, email FROM users LIMIT 10")
    rows = await cursor.fetchall()
    await cursor.close()
    await db_pool.release(conn)
    return Response.json(rows_to_json(rows))
```

### 5. In-Memory Testing (`TestClient`)
Test your entire application without opening a network socket:
```python
from codon_web import TestClient

client = TestClient(app)
res = client.get("/users/123?category=books")

assert res.status_code == 200
assert "active" in res.text
```

---

## 📂 Repository Structure

```
codon_web/
├── codon_web.codon        # Top-level public API exports
├── Makefile               # Project build, native C compilation, and test automation
├── src/
│   ├── framework/         # Core framework: App, Router, Radix Tree, OpenAPI, Security, DI
│   ├── http/              # HTTP Parser, Request/Response, FastJson, Schema, Params, Streaming
│   ├── db/                # MySQL (sync/async) & SQLite drivers, Connection Pools
│   ├── runtime/           # Linux Epoll event reactor, non-blocking socket loop
│   ├── ws/                # RFC 6455 WebSocket protocol (frames, handshake, crypto)
│   ├── memory/            # ByteSlice zero-copy memory abstractions
│   ├── native/            # C native acceleration: yyjson, libcfastjson.a/so
│   └── testing/           # In-memory TestClient and assertions
├── examples/              # Full-featured applications, FastAPI parity demos, benchmarks
├── tests/                 # Comprehensive test suites (26 integration and unit tests)
│   ├── benchmark/         # JSON and server performance benchmark harnesses
│   └── run_all.codon      # Master test runner
└── docs/                  # Architecture documentation and benchmark specifications
    ├── architecture.md
    └── benchmarks.md
```

---

## 🧪 Testing

Run the full automated test suite:
```bash
# Build native C acceleration libraries
make native

# Run integration tests
make test

# Run complete 26-test suite
make test-all
```

---

## 🤝 Contributing

Contributions are warmly welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on code style, development guidelines, and pull request workflows.

---

## 📄 License

CodonWeb is open-source software licensed under the [MIT License](LICENSE).
