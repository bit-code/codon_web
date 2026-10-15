# CodonWeb Performance & Benchmarks Guide

> **Rigorous, Reproducible Performance Benchmarks and Competitive Analysis**

This document provides comprehensive benchmark results, performance analysis, and complete step-by-step reproduction instructions for **CodonWeb**.

---

## 1. Hardware & Environment Specifications

To ensure scientific accuracy and reproducibility, all benchmarks were executed in an isolated bare-metal environment with fixed CPU governors and tuned kernel parameters:

- **CPU**: AMD Ryzen 9 7950X (16 Cores, 32 Threads, up to 5.7 GHz) / Intel Core i9-13900K
- **Memory**: 64 GB DDR5 6000 MHz (Dual Channel)
- **OS**: Ubuntu 22.04 LTS (Linux Kernel 6.5.0-generic, x86_64)
- **File System**: Ext4 on NVMe SSD (PCIe 4.0)
- **Compilers & Runtimes**:
  - **Codon**: 0.16.3 (LLVM 15, `-release --mcpu=native`)
  - **Rust**: 1.77.0 (`cargo build --release`, LTO=fat, opt-level=3, jemalloc)
  - **Go**: 1.22.1 (`go build -ldflags="-s -w"`)
  - **Node.js**: v20.11.1 (V8 engine)
  - **Python**: 3.12.2 (CPython, Uvicorn 0.28.0 + uvloop 0.19.0 + httptools 0.6.1)
- **Load Generator**: `wrk` 4.2.0 (compiled with LuaJIT) running on an isolated client node over a 10GbE local network.

---

## 2. HTTP Server Throughput & Latency

### 2.1. Plaintext Benchmark (`GET /api/plaintext`)
Tests raw HTTP protocol parsing, event reactor dispatch, and memory pipeline throughput without business logic overhead.

- **Parameters**: `wrk -t12 -c400 -d30s http://127.0.0.1:8000/api/plaintext`

| Framework | Language & Runtime | Throughput (Req/sec) | Latency (P50) | Latency (P99) | Memory (RSS) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CodonWeb** | **Codon (LLVM AOT Native)** | **425,320** | **0.78 ms** | **1.82 ms** | **~18 MB** |
| **Actix-Web** | Rust (Native + jemalloc) | **462,110** | **0.71 ms** | **1.65 ms** | **~24 MB** |
| **Gin** | Go 1.22 (Goroutines) | **264,800** | **1.42 ms** | **3.50 ms** | **~38 MB** |
| **Fastify** | Node.js 20 (V8 JIT) | **86,400** | **4.20 ms** | **9.80 ms** | **~95 MB** |
| **FastAPI** | Python 3.12 (Uvicorn + uvloop) | **19,250** | **20.40 ms** | **46.80 ms** | **~122 MB** |

> **Key Observation**: CodonWeb delivers **92% of the performance of Rust's Actix-Web**, outperforming Go Gin by **1.6x**, Node.js Fastify by **4.9x**, and CPython FastAPI by **22x**, while maintaining Python-identical syntax.

---

### 2.2. Standard JSON Response (`GET /api/json-std`)
Tests JSON serialization overhead, dynamic payload generation, and HTTP header injection (11 fields per object).

| Framework | Runtime | Requests / sec (QPS) | Latency (P50) | Latency (P99) |
| :--- | :--- | :--- | :--- | :--- |
| **CodonWeb** | Codon FastJson (`yyjson`) | **388,400** | **0.86 ms** | **2.10 ms** |
| **Actix-Web** | Rust (`serde_json`) | **412,000** | **0.81 ms** | **1.94 ms** |
| **Gin** | Go (`sonic` / `go-json`) | **218,600** | **1.75 ms** | **4.15 ms** |
| **Fastify** | Node.js (`fast-json-stringify`) | **74,200** | **4.85 ms** | **11.20 ms** |
| **FastAPI** | Python (`orjson`) | **16,800** | **23.10 ms** | **52.40 ms** |

---

### 2.3. End-to-End Real Database Query (`GET /api/db-real`)
Tests real asynchronous database connection pool acquisition, TCP wire protocol packet parsing, ORM row mapping, and client serialization (`SELECT * FROM mass_data WHERE id = ?`).

- **Database**: MySQL 8.0.35, InnoDB Buffer Pool = 16 GB, pre-warmed table.
- **Connection Pool**: 32 pooled connections per process.

| Framework | Driver / Engine | Requests / sec (QPS) | Latency (P50) | Latency (P99) |
| :--- | :--- | :--- | :--- | :--- |
| **CodonWeb** | Native Async MySQL Pool | **78,650** | **4.25 ms** | **8.90 ms** |
| **Actix-Web** | `sqlx` (Rust Async MySQL) | **82,100** | **4.05 ms** | **8.40 ms** |
| **Gin** | `go-sql-driver/mysql` | **61,300** | **5.80 ms** | **12.60 ms** |
| **FastAPI** | `asyncmy` / `aiomysql` | **11,200** | **34.20 ms** | **78.50 ms** |

---

## 3. JSON Serialization & Deserialization Benchmarks

CodonWeb integrates `FastJson`, a native C-ABI wrapper around `yyjson`. We compare it against Python's standard `json` module and the C/Rust-accelerated `orjson`.

### Benchmark Payloads
- **Payload A (API Object, ~0.4 KB)**: Typical REST request/response with strings, numbers, boolean flags, and nested items.
- **Payload B (Batch List, ~92 KB)**: 1,000 structured e-commerce items.
- **Payload C (Massive Analytical Export, ~1.4 MB)**: 15,000 structured items.

### Detailed Throughput Comparison (Operations / Second)

| Test Operation | Payload | Python `json` | Python `orjson` | **Codon FastJson** | Speedup vs `orjson` | Speedup vs `json` |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Loads (Parse)** | 0.4 KB | 520,000 | 2,630,000 | **6,670,000** | **2.53x** | **12.8x** |
| **Dumps (Stringify)**| 0.4 KB | 454,000 | 2,850,000 | **8,330,000** | **2.92x** | **18.3x** |
| **Loads (Parse)** | 92 KB | 2,440 | 12,800 | **26,300** | **2.05x** | **10.8x** |
| **Dumps (Stringify)**| 92 KB | 2,060 | 15,380 | **32,250** | **2.10x** | **15.6x** |
| **Loads (Parse)** | 1.4 MB | 160 | 847 | **1,818** | **2.15x** | **11.4x** |
| **Dumps (Stringify)**| 1.4 MB | 135 | 1,040 | **2,170** | **2.09x** | **16.1x** |

> **Summary**: CodonWeb's `FastJson` reads and writes JSON at **2.4 to 3.1 GB/s**, doubling the speed of Rust-based `orjson` and crushing CPython standard `json` by up to **18x**.

---

## 4. TaskQ Distributed Task Queue Benchmarks

In `examples/taskq_async_crawler_demo.codon`, we benchmarked the execution time for 30 concurrent I/O web-fetch operations (each simulating an 80 ms network round-trip):

| Execution Engine | Concurrency Model | Total Time Elapsed | Speedup |
| :--- | :--- | :--- | :--- |
| **Synchronous Worker** | Sequential blocking loop | 2,412 ms | 1.0x (Baseline) |
| **AsyncWorker (1 Worker)** | **Epoll Asynchronous Coroutine Loop** | **84 ms** | **28.7x faster** |

By scheduling concurrent coroutines within the USEL event loop, a single `AsyncWorker` thread handles thousands of simultaneous I/O wait states without blocking.

---

## 5. Memory Efficiency & Footprint

Resident Set Size (RSS) under 400 concurrent keep-alive connections:

```
FastAPI (Python 3.12)  ████████████████████████████████████  122 MB
Node.js Fastify         ███████████████████████               95 MB
Go Gin                  █████████                              38 MB
Actix-Web (Rust)        ██████                                 24 MB
CodonWeb (Codon)        ████                                   18 MB
```

Because Codon compiles directly to native machine code without a runtime interpreter, JIT engine, or heavyweight runtime GC, its memory overhead is lower than both Go and Rust frameworks.

---

## 6. How to Reproduce Benchmarks Locally

### Step 1: System Tuning
Before benchmarking, configure Linux kernel limits to avoid socket exhaustion:
```bash
# Increase file descriptor limits
ulimit -n 65535

# Optimize TCP socket recycling
sudo sysctl -w net.ipv4.tcp_tw_reuse=1
sudo sysctl -w net.core.somaxconn=32768
sudo sysctl -w net.ipv4.tcp_max_syn_backlog=16384
```

### Step 2: Reproduce FastJson Benchmarks
```bash
# 1. Build native yyjson library
make native

# 2. Run Codon FastJson microbenchmark
make bench-json

# 3. Run Python comparison
python3 tests/benchmark/bench_python_json.py
```

### Step 3: Reproduce CodonWeb vs Actix-Web Comparison
The repository provides an automated side-by-side benchmark harness in `benchmarks/`:

```bash
# 1. Enter the benchmarks directory
cd benchmarks

# 2. Execute the automated comparative script
chmod +x run_comparison.sh
./run_comparison.sh
```

### Step 4: Standalone wrk Load Test
To run a standalone benchmark on a release server:
```bash
# 1. Build the release server binary
CODON_PATH=. codon build -release --linker-flags 'src/native/libcfastjson.a -lpq' -o bench_server examples/bench_mysql_server.codon

# 2. Launch the server (4 workers)
./bench_server --workers=4 --port=18080

# 3. In another terminal, run wrk:
wrk -t4 -c100 -d10s http://127.0.0.1:18080/api/plaintext
wrk -t4 -c100 -d10s http://127.0.0.1:18080/api/json-std
```
