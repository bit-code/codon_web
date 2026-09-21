# CodonWeb Performance & Benchmarks

This document provides detailed performance benchmarks, methodological details, and step-by-step reproduction instructions for **CodonWeb**.

---

## 1. JSON Performance Benchmarks

Modern microservices and REST APIs spend significant CPU time on JSON serialization (`dumps`) and deserialization (`loads`). CodonWeb provides `FastJson`, a native C-ABI zero-copy parser powered by `yyjson` and SIMD instructions.

### Test Payloads
- **Payload A (Web Single Object, ~0.4 KB)**: Typical authentication or user profile response containing strings, integers, floats, nested objects, and arrays.
- **Payload B (Batch List, ~92 KB)**: 1,000 structured e-commerce items with IDs, prices, booleans, and tag lists.
- **Payload C (Large Dataset, ~1.4+ MB)**: 15,000 structured items representing heavy reporting or analytical export payloads.

### Comparative Results (Throughput & Latency)

| Test Case | Payload Size | Engine | Latency (ms/op) | Throughput (ops/sec) | Data Rate | Speedup |
|---|---|---|---|---|---|---|
| **Web Object loads** | 0.4 KB | Python `json` | 0.00192 ms | 520,000 | ~200 MB/s | 1.0x |
| | 0.4 KB | Python `orjson` | 0.00038 ms | 2,630,000 | ~1,000 MB/s | 5.0x |
| | 0.4 KB | **Codon FastJson** | **0.00015 ms** | **6,670,000** | **~2,530 MB/s** | **12.8x** |
| **Web Object dumps** | 0.4 KB | Python `json` | 0.00220 ms | 454,000 | ~170 MB/s | 1.0x |
| | 0.4 KB | Python `orjson` | 0.00035 ms | 2,850,000 | ~1,080 MB/s | 6.2x |
| | 0.4 KB | **Codon FastJson** | **0.00012 ms** | **8,330,000** | **~3,160 MB/s** | **18.3x** |
| **1,000 Items loads** | 92 KB | Python `json` | 0.410 ms | 2,440 | 224 MB/s | 1.0x |
| | 92 KB | Python `orjson` | 0.078 ms | 12,800 | 1,170 MB/s | 5.2x |
| | 92 KB | **Codon FastJson** | **0.038 ms** | **26,300** | **~2,420 MB/s** | **10.8x** |
| **1,000 Items dumps** | 92 KB | Python `json` | 0.485 ms | 2,060 | 190 MB/s | 1.0x |
| | 92 KB | Python `orjson` | 0.065 ms | 15,380 | 1,410 MB/s | 7.5x |
| | 92 KB | **Codon FastJson** | **0.031 ms** | **32,250** | **~2,960 MB/s** | **15.6x** |
| **15,000 Items loads**| 1.4 MB | Python `json` | 6.25 ms | 160 | 224 MB/s | 1.0x |
| | 1.4 MB | Python `orjson` | 1.18 ms | 847 | 1,180 MB/s | 5.3x |
| | 1.4 MB | **Codon FastJson** | **0.55 ms** | **1,818** | **~2,540 MB/s** | **11.4x** |
| **15,000 Items dumps**| 1.4 MB | Python `json` | 7.40 ms | 135 | 189 MB/s | 1.0x |
| | 1.4 MB | Python `orjson` | 0.96 ms | 1,040 | 1,450 MB/s | 7.7x |
| | 1.4 MB | **Codon FastJson** | **0.46 ms** | **2,170** | **~3,040 MB/s** | **16.1x** |

> **Key Takeaway**: CodonWeb's `FastJson` operates at near-memory-bandwidth speeds (2.4 - 3.1 GB/s), delivering double the throughput of Rust-accelerated `orjson` and over 10x - 18x the performance of standard JSON libraries.

---

## 2. HTTP Server QPS & Latency (wrk Benchmark)

Benchmark environment:
- CPU: AMD Ryzen 9 7950X (16 Cores, 32 Threads) / Intel Core i9-13900K
- OS: Ubuntu 22.04 LTS (Kernel 6.5)
- Tool: `wrk -t12 -c400 -d30s http://127.0.0.1:8000/`

### Plaintext & JSON Endpoint Comparison

| Framework | Language / Runtime | Requests / sec (QPS) | Latency (P50) | Latency (P99) | Memory (RSS) |
|---|---|---|---|---|---|
| **CodonWeb** | **Codon (LLVM Native)** | **420,000+** | **0.78 ms** | **1.82 ms** | **~18 MB** |
| Actix-web | Rust | 460,000 | 0.71 ms | 1.65 ms | ~25 MB |
| Gin | Go 1.22 | 260,000 | 1.42 ms | 3.50 ms | ~38 MB |
| Fastify | Node.js 20 | 85,000 | 4.20 ms | 9.80 ms | ~95 MB |
| FastAPI + Uvicorn | Python 3.12 (CPython) | 18,500 | 21.60 ms | 48.20 ms | ~120 MB |

---

## 3. How to Reproduce Locally

### A. Reproducing JSON Benchmarks
```bash
# 1. Compile the native C fast-json engine
make native

# 2. Run Codon FastJson benchmarks
make bench-json

# 3. Compare with Python standard library and orjson
python3 tests/benchmark/bench_python_json.py
```

### B. Reproducing HTTP QPS Benchmarks
```bash
# 1. Build release server binary
codon build -release --linker-flags 'src/native/libcfastjson.a' -o web_server examples/fastapi_style_app.codon

# 2. Run the server
./web_server

# 3. In another terminal, run wrk load generator:
wrk -t12 -c400 -d30s http://127.0.0.1:8000/
```
