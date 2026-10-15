# CodonWeb vs Actix-Web 吞吐量对比基准测试

本项目提供了与 [bench_mysql_server.codon](../../examples/bench_mysql_server.codon) 100% 对齐的 **Actix-Web (Rust Native)** 高性能压测服务端实现，位于目录 `benchmarks/actix_mysql_server`。

---

## 一、 测试接口对齐说明

| 接口路由 | 协议与功能 | 性能考验点 |
| :--- | :--- | :--- |
| **`GET /api/plaintext`** | HTTP 200 返回 `Hello, World!` | HTTP 协议栈解析、事件循环调度、纯吞吐极限 |
| **`GET /api/json-std`** | 返回标准业务 JSON 对象（11 个字段） | 序列化引擎 (yyjson vs serde_json)、内存分配 |
| **`GET /api/db-real`** | 执行 `SELECT * FROM mass_data WHERE id=1022578` | 异步连接池调度、MySQL Wire 协议解析、ORM/Row 映射 |

---

## 二、 编译与启动服务

### 1. CodonWeb 服务端 (端口 18080)
```bash
# 1. 编译 CodonWeb Release 二进制
CODON_PATH=. codon build -release --linker-flags 'src/native/libcfastjson.a' -o bench_mysql_server examples/bench_mysql_server.codon

# 2. 启动服务 (4 Worker, 监听 18080)
./bench_mysql_server --workers=4 --port=18080
```

### 2. Actix-Web 服务端 (端口 18081)
已开启 **LTO (Link Time Optimization)**、**opt-level=3** 与 **jemalloc** 全局无锁内存分配器。

```bash
# 1. 编译 Actix-Web Release 二进制 (已完成构建)
cd benchmarks/actix_mysql_server
cargo build --release

# 2. 启动服务 (4 Worker, 监听 18081)
./target/release/actix_mysql_server --workers=4 --port=18081
```

---

## 三、 Wrk 压测对比命令

> 建议在测试前将系统文件句柄限制调大：`ulimit -n 65535`

### 1. 纯文本基准 (`/api/plaintext`)
```bash
# CodonWeb
wrk -t4 -c100 -d10s http://127.0.0.1:18080/api/plaintext

# Actix-Web
wrk -t4 -c100 -d10s http://127.0.0.1:18081/api/plaintext
```

### 2. 标准 JSON 基准 (`/api/json-std`)
```bash
# CodonWeb
wrk -t4 -c100 -d10s http://127.0.0.1:18080/api/json-std

# Actix-Web
wrk -t4 -c100 -d10s http://127.0.0.1:18081/api/json-std
```

### 3. 真实 MySQL 查询基准 (`/api/db-real`)
```bash
# CodonWeb
wrk -t4 -c100 -d10s http://127.0.0.1:18080/api/db-real

# Actix-Web
wrk -t4 -c100 -d10s http://127.0.0.1:18081/api/db-real
```
