# CodonWeb

<p align="center">
  <strong style="font-size: 1.4em;">Next-Generation, Native-Compiled High-Performance Web Framework for Codon (LLVM)</strong>
</p>

<p align="center">
  <a href="https://github.com/bit-code/codon_web/actions"><img src="https://img.shields.io/badge/CI-Passing-brightgreen?style=flat-square&logo=githubactions" alt="CI Status"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue?style=flat-square" alt="License"></a>
  <a href="https://github.com/exaloop/codon"><img src="https://img.shields.io/badge/compiler-Codon%20(LLVM)-blueviolet?style=flat-square" alt="Codon"></a>
  <img src="https://img.shields.io/badge/throughput-425k%2B%20QPS-orange?style=flat-square" alt="Throughput">
  <img src="https://img.shields.io/badge/memory-~18MB%20RSS-green?style=flat-square" alt="Memory">
  <a href="README_zh.md"><img src="https://img.shields.io/badge/docs-%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87-blue?style=flat-square" alt="Chinese Documentation"></a>
</p>

<p align="center">
  <a href="README.md">English</a> | <a href="README_zh.md">简体中文</a>
</p>

---

## ⚡ What is CodonWeb?

**CodonWeb** is an ahead-of-time (AOT) compiled web framework and runtime built for [Codon](https://github.com/exaloop/codon). It compiles pure, elegant Pythonic code directly into standalone, native machine code via LLVM.

By eliminating the Python Global Interpreter Lock (GIL), dynamic reflection, and interpreter overhead, CodonWeb delivers the **raw speed and memory efficiency of C++ and Rust (Actix-Web)** while preserving the **developer ergonomics of FastAPI and SQLAlchemy**.

```
              ┌─────────────────────────────────────────────────────────────┐
              │                   FastAPI-Style Elegance                    │
              │         (Decorators, Type Hints, Depends, OpenAPI)          │
              └──────────────────────────────┬──────────────────────────────┘
                                             │ Ahead-of-Time (AOT)
                                             ▼
              ┌─────────────────────────────────────────────────────────────┐
              │                 Codon + LLVM Optimization                   │
              │         (Native Machine Code, Inlining, Zero-GIL)           │
              └──────────────────────────────┬──────────────────────────────┘
                                             │ Direct Kernel Integration
                                             ▼
              ┌─────────────────────────────────────────────────────────────┐
              │                Rust / C++ Execution Velocity                │
              │        (425,000+ QPS, 0.7ms Latency, 18MB Footprint)        │
              └─────────────────────────────────────────────────────────────┘
```

---

## 🚀 Key Highlights

- 🏎️ **Bare-Metal Throughput**: Exceeds **425,000 requests/sec** with sub-millisecond P50 latency (~0.78 ms) on standard multi-core hardware.
- 🧬 **Code-First AOT Schema Engine**: Type-safe request schemas (`BaseModel`) compiled to native deserialization and validation routines with zero runtime reflection.
- ⚡ **SIMD FastJson (`yyjson`)**: Direct C-ABI SIMD acceleration processing JSON at **2.4 to 3.1 GB/s** (12x faster than Python `json`, 2x faster than `orjson`).
- 🗄️ **Industrial ORM & SQL Builder**: Dual-dialect (MySQL & PostgreSQL) SQLAlchemy-style expression builder (`select().where().order_by()`) and non-blocking connection pools.
- 📦 **TaskQ Distributed Task Queue**: Celery-like native task queue featuring Redis brokers, delayed scheduling (ZSet), exponential retry, and asynchronous coroutine workers (`AsyncWorker`).
- 🔄 **Unified Single Event Loop (USEL)**: Native Linux Epoll and `io_uring` reactors with zero-copy TCP buffer slicing (`ByteSlice`).
- 🛡️ **Production-Ready Onion Middleware**: Comprehensive middleware stack including CORS, Sliding-Window & Per-IP Rate Limiting, HMAC-SHA256 Sessions, Security Headers, and Panic Recovery.
- 💬 **Real-time Protocols**: RFC 6455 WebSockets, Server-Sent Events (SSE), and HTTP/1.1 Chunked Transfer Streaming.
- 📜 **Automated OpenAPI 3.0**: Built-in interactive Swagger UI (`/docs`) and ReDoc (`/redoc`) generation.
- 🧪 **Zero-Socket `TestClient`**: High-speed in-memory integration testing without binding physical TCP ports.

---

## 📊 Performance Benchmarks

### 1. HTTP Server Throughput (`wrk` 400 Concurrency, 12 Threads)

| Framework | Language & Runtime | Throughput (Req/sec) | Latency (P50) | Latency (P99) | Memory (RSS) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CodonWeb** | **Codon (LLVM AOT Native)** | **455,320** | **0.78 ms** | **1.82 ms** | **~18 MB** |
| **Actix-Web** | Rust (Native + jemalloc) | **462,110** | **0.71 ms** | **1.65 ms** | **~24 MB** |
| **Gin** | Go 1.22 (Goroutines) | **264,800** | **1.42 ms** | **3.50 ms** | **~38 MB** |
| **Fastify** | Node.js 20 (V8 JIT) | **86,400** | **4.20 ms** | **9.80 ms** | **~95 MB** |
| **FastAPI** | Python 3.12 (Uvicorn + uvloop) | **19,250** | **20.40 ms** | **46.80 ms** | **~122 MB** |

### 2. JSON Serialization & Deserialization Speed

| Test Payload | Python standard `json` | Python `orjson` | **Codon FastJson (`yyjson`)** | Speedup |
| :--- | :--- | :--- | :--- | :--- |
| **API Object (0.4 KB)** | 520k ops/s | 2,630k ops/s | **6,670k ops/s** | **12.8x faster** |
| **Batch List (92 KB)** | 2,440 ops/s | 12,800 ops/s | **26,300 ops/s** | **10.8x faster** |
| **Large Export (1.4 MB)**| 160 ops/s | 847 ops/s | **1,818 ops/s** | **11.4x faster** |

*For full benchmark methodologies and reproduction scripts, see [docs/benchmarks.md](docs/benchmarks.md).*

---

## 📦 Quick Start

### 1. Prerequisites
- **Operating System**: Linux (Ubuntu 20.04+ recommended), Windows WSL2, or macOS.
- **Build Tools**: `gcc` (or `clang`), `make`, `libpq-dev`.
- **Codon Compiler**:
  ```bash
  curl -fsSL https://exaloop.io/install.sh | bash
  echo 'export PATH="$HOME/.codon/bin:$PATH"' >> ~/.bashrc
  source ~/.bashrc
  ```

### 2. Clone & Build Native Extensions
```bash
git clone https://github.com/bit-code/codon_web.git
cd codon_web

# Compile native C SIMD acceleration libraries (libcfastjson.a, picohttpparser, libpq)
make native
```

### 3. Create Your First App (`main.codon`)
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
    return Response.json('{"message": "Hello from native compiled CodonWeb!"}')

@app.get("/users/{user_id}")
def get_user(req: Request) -> Response:
    user_id = req.param("user_id")
    return Response.json('{"id": ' + user_id + ', "status": "active"}')

if __name__ == "__main__":
    app.run()
```

### 4. Compile Ahead-of-Time and Run
```bash
# Compile to a standalone optimized native binary
CODON_PATH=. codon build -release --linker-flags 'src/native/libcfastjson.a -lpq' -o app main.codon

# Execute standalone binary
./app
```
Explore interactive Swagger UI at [http://localhost:8000/docs](http://localhost:8000/docs)!

---

## 🛠️ Feature Showcase

### 1. Code-First AOT Schema & 422 Validation
Define models using clean Python class syntax:
```python
from src.schema import BaseModel, StrField, IntField, FloatField

class UserCreate(BaseModel):
    username: str = StrField(min_length=3, max_length=32)
    age: int = IntField(ge=18, le=120)
    email: str = StrField(format="email")
    balance: float = FloatField(ge=0.0, default=0.0)
```
Compile schemas with `make codegen`. In your route:
```python
@app.post("/api/users")
def register_user(req: Request) -> Response:
    res = UserCreate.from_json_doc(req.json_doc())
    if not res.is_ok():
        return res.to_response(422)  # Structured RFC 7807 error
    
    user = res.unwrap()
    return Response.json('{"created": "' + user.username + '"}', 201)
```

### 2. SQLAlchemy-Style Fluent Query Builder
Write SQL with compile-time type safety and multi-dialect compatibility:
```python
from codon_web.orm import Table, select, and_, MYSQL_DIALECT, POSTGRESQL_DIALECT

users = Table("users")

query = (
    select(users.id, users.username, users.email)
    .select_from(users)
    .where(
        users.is_active == True,
        users.age >= 18
    )
    .order_by(users.id.desc())
    .limit(20)
)

# Automatically formats for MySQL (backticks, ?) or PostgreSQL (double quotes, $1)
sql, params = query.to_sql(MYSQL_DIALECT)
```

### 3. TaskQ: Distributed Task Queue
Offload heavy or high-concurrency workloads to background workers:
```python
from codon_web import TaskClient
from src.taskq.broker.redis import RedisBroker

broker = RedisBroker("127.0.0.1", 6379)
client = TaskClient(broker)

# Enqueue background task with 0.1ms dispatch time
async_res = client.delay("generate_report", '{"user_id": 10086}')
print("Dispatched Task ID:", async_res.task_id)
```
*A single `AsyncWorker` processes up to **28x more concurrent I/O jobs** than standard synchronous workers (see [docs/taskq.md](docs/taskq.md)).*

### 4. Dependency Injection (`Depends`)
```python
from codon_web import Request, HTTPBearer, HTTPException

security = HTTPBearer()

def get_current_user(req: Request) -> str:
    token = security(req)
    if token != "secret-token":
        raise HTTPException(401, "Invalid or expired credentials")
    return "authorized_user"

@app.get("/api/profile")
def profile(req: Request) -> Response:
    user = get_current_user(req)
    return Response.json('{"user": "' + user + '"}')
```

### 5. RFC 6455 Full WebSocket Protocol
```python
from codon_web import App, Request, ws

app = App()

@app.websocket("/ws/chat")
def chat_socket(req: Request, conn: ws.WebSocketConnection):
    while True:
        frame = conn.read_frame()
        if frame.opcode == ws.OP_CLOSE:
            break
        elif frame.opcode == ws.OP_TEXT:
            conn.send_text("Echo: " + frame.payload)
```

### 6. Zero-Socket In-Memory Testing (`TestClient`)
```python
from codon_web import TestClient

client = TestClient(app)
res = client.get("/users/123")

assert res.status_code == 200
assert "status" in res.text
```

---

## 📂 Repository Structure

```
codon_web/
├── codon_web.codon             # Top-level unified API exports
├── Makefile                    # Build automation, native C compilation, and tests
├── src/
│   ├── framework/              # App engine, Radix tree router, OpenAPI, Dependency Injection
│   ├── http/                   # PicoHTTPParser, Request/Response, FastJson, Streaming, SSE
│   ├── schema/                 # Code-First AOT Schema subsystem, AST validators, 422 errors
│   ├── orm/                    # Dialects (MySQL/PostgreSQL), Models, SQLAlchemy-style SQL Builder
│   ├── db/                     # Native MySQL & PostgreSQL async pools, SQLite, Redis drivers
│   ├── taskq/                  # Celery-like distributed task queue (Sync & AsyncWorker)
│   ├── runtime/                # Linux Epoll reactor, USEL event loop, io_uring, date cache
│   ├── ws/                     # RFC 6455 WebSocket protocol (frames, handshake, crypto)
│   ├── memory/                 # ByteSlice zero-copy memory abstractions
│   ├── native/                 # C-ABI acceleration: yyjson, picohttpparser, libpq, io_uring
│   └── testing/                # In-memory TestClient and assertion test framework
├── tools/                      # AOT Schema code generators (AST + Jinja2)
├── benchmarks/                 # CodonWeb vs Actix-Web (Rust) side-by-side benchmark harness
├── examples/                   # Production-grade examples (FastAPI style, AOT Schema, TaskQ)
├── tests/                      # 33 comprehensive unit and integration test suites
└── docs/                       # In-depth architectural & developer guides
    ├── architecture.md         # Comprehensive system architecture & memory model
    ├── benchmarks.md           # Performance benchmarks & reproduction instructions
    ├── schema_guide.md         # Code-First AOT Schema guide & validation
    ├── orm_guide.md            # ORM, Session lifecycle & Fluent SQL Builder
    ├── taskq.md                # TaskQ distributed queue architecture & usage
    └── deployment.md           # Production deployment, Systemd, Nginx & Docker
```

---

## 📖 In-Depth Documentation

- 🏛️ [System Architecture & Memory Model](docs/architecture.md)
- 📊 [Performance Benchmarks & Reproduction Guide](docs/benchmarks.md)
- 🧬 [Code-First AOT Schema Guide](docs/schema_guide.md)
- 🗄️ [ORM & Fluent SQL Query Builder Guide](docs/orm_guide.md)
- 📦 [TaskQ Distributed Task Queue Guide](docs/taskq.md)
- 🚢 [Production Deployment, Systemd & Docker Guide](docs/deployment.md)

---

## 🧪 Testing

Run the full automated test suite (33 test suites):
```bash
# 1. Compile native C libraries
make native

# 2. Run core integration tests
make test

# 3. Run all 33 unit and integration test suites
make test-all

# 4. Run AOT schema tests
make test-schema

# 5. Run TaskQ distributed queue tests
make test-taskq
```

---

## 🤝 Contributing

We warmly welcome community contributions! Please review [CONTRIBUTING.md](CONTRIBUTING.md) for code style standards, local environment setup, and pull request guidelines.

---

## 📄 License

CodonWeb is open-source software licensed under the [MIT License](LICENSE).
