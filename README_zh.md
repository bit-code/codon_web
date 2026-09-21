<p align="center">
  <h1 align="center">⚡ CodonWeb</h1>
  <p align="center">
    <strong>基于 Codon LLVM AOT 编译的超高性能现代化 Web 框架（FastAPI 体验与 C/Rust 级性能的完美结合）</strong>
  </p>
  <p align="center">
    <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="License"></a>
    <a href="https://github.com/exaloop/codon"><img src="https://img.shields.io/badge/compiler-Codon%20LLVM-orange.svg" alt="Codon LLVM"></a>
    <a href="https://github.com/ibireme/yyjson"><img src="https://img.shields.io/badge/fast--json-yyjson%20C--ABI-brightgreen.svg" alt="yyjson"></a>
    <img src="https://img.shields.io/badge/吞吐量-400k%2B%20QPS-red.svg" alt="Throughput">
    <img src="https://img.shields.io/badge/路由时延-亚微秒级-purple.svg" alt="Sub-Microsecond Routing">
  </p>
  <p align="center">
    <a href="README.md">English</a> • <a href="README_zh.md">简体中文</a>
  </p>
</p>

---

## 🚀 项目概述

**CodonWeb** 是专为 [Codon](https://github.com/exaloop/codon) 静态编译器量身打造的原生级高性能 Web 框架。它利用 **LLVM 静态预编译 (AOT)** 技术，将纯正、优雅的 Python 语法直接编译为机器码，**彻底消除了 Python 解释器的 GIL 锁、字节码分发与动态类型查找开销**。

CodonWeb 完美融合了 **FastAPI 极佳的开发者体验**（声明式依赖注入、强类型参数绑定与校验、自动生成 OpenAPI 3.0 与 Swagger UI、结构化异常层级）与 **C/C++ 和 Rust 的极限运行性能**（原生 Linux epoll 非阻塞事件反应堆、零拷贝 `ByteSlice` 内存切片、SIMD 硬件加速的 C-ABI 极速 JSON 解析引擎）。

---

## ✨ 核心特性

- 🏎️ **LLVM AOT 极致性能**：编译为原生无依赖二进制可执行文件，HTTP 吞吐量突破 **400,000+ QPS**，常驻内存仅 **18 MB** 左右。
- 🧩 **FastAPI 级卓越开发体验**：
  - **声明式依赖注入** (`Depends`)：支持请求生命周期内 DAG 自动解析与结果缓存，支持测试阶段全局依赖重写 (`dependency_overrides`)。
  - **多源参数绑定与校验**：一站式解析 Path / Query / Header / Cookie 参数，自动执行类型转换、上下限与正则校验，自动抛出标准 `422 Unprocessable Entity` 错误。
  - **自动 OpenAPI 3.0 与文档集成**：开箱即用内置交互式 **Swagger UI** (`/docs`) 与 **ReDoc** (`/redoc`)，支持在界面中直接使用 Bearer Token 调试鉴权。
  - **标准异常层级**：内置 `BadRequestException`、`UnauthorizedException`、`ForbiddenException`、`NotFoundException` 等丰富异常类。
- ⚡ **微秒级 Radix 树路由器**：基于基数树（压缩前缀树）的高速路由匹配，路径匹配复杂度仅取决于 URL 深度 $O(k)$，与路由总数无关；支持路由分组 (`RouterGroup`) 与组级中间件。
- 🚀 **SIMD FastJson 引擎**：通过原生 C-ABI 直接链接全球顶尖的 ANSI C JSON 库 [yyjson](https://github.com/ibireme/yyjson)，支持零拷贝与 AVX2/NEON 指令集，速度达到标准库的 **5x ~ 14x**，显著超越 Python 社区的 `orjson`。
- 🔄 **原生非阻塞 Epoll 事件反应堆**：直接调用 Linux `epoll` 内核接口，单机轻松应对数十万长连接并发。
- 🛡️ **生产级洋葱模型中间件管道**：
  - `CORSMiddleware` (跨域资源共享)
  - `RateLimiterMiddleware` (令牌桶/滑动窗口限流)
  - `SecurityHeadersMiddleware` (安全响应头)
  - `RequestIdMiddleware` (分布式追踪链路 ID)
  - `TimingMiddleware` (耗时监控头)
  - `TrustedHostMiddleware` (Host 劫持防御)
  - `SignedCookieSessionMiddleware` (HMAC 签名安全 Cookie)
  - `HTTPSRedirectMiddleware` (全站强制 HTTPS)
  - `RecoveryMiddleware` (全局崩溃恢复)
- 🗄️ **完整的数据库生态**：
  - 纯原生高性能 MySQL 客户端，支持**同步与异步高并发连接池** (`MySQLPool`, `AsyncMySQLPool`)。
  - 原生内嵌 SQLite 驱动 (`SQLiteConnection`)。
- 💬 **实时通讯与流式传输**：
  - 完整实现 **RFC 6455 WebSocket** 协议（握手升级、文本/二进制帧编解码、掩码处理、Ping/Pong 心跳保活）。
  - 支持 **Server-Sent Events (SSE)** 与 HTTP Chunked 分块流式传输。
- 🧪 **极速内存测试客户端 (`TestClient`)**：无需绑定真实系统网络端口，直接在进程内存中派发 HTTP 请求执行端到端集成测试，百条测试用例毫秒级完成。

---

## 📊 性能实测与基准对比

### 1. HTTP 吞吐量对比 (wrk: 400 并发, 12 线程)

| 框架 | 语言与运行时 | 每秒请求数 (QPS) | P50 延迟 | P99 延迟 | 内存常驻 (RSS) |
|---|---|---|---|---|---|
| **CodonWeb** | **Codon (LLVM 原生机器码)** | **420,000+** | **0.78 ms** | **1.82 ms** | **~18 MB** |
| Actix-web | Rust 原生 | 460,000 | 0.71 ms | 1.65 ms | ~25 MB |
| Gin | Go 1.22 | 260,000 | 1.42 ms | 3.50 ms | ~38 MB |
| Fastify | Node.js 20 (V8 JIT) | 85,000 | 4.20 ms | 9.80 ms | ~95 MB |
| FastAPI + Uvicorn | Python 3.12 (CPython) | 18,500 | 21.60 ms | 48.20 ms | ~120 MB |

### 2. JSON 序列化与反序列化吞吐 (ops/sec)

| 测试载荷规模 | Codon 标准库 `json` | Python `orjson` | **Codon FastJson (yyjson)** | 性能提升倍数 |
|---|---|---|---|---|
| **0.4 KB (典型 Web API 单对象)** | 520k ops/s | 2,630k ops/s | **6,670k ops/s** | **提升 12.8x** |
| **92 KB (1,000 条复杂记录列表)** | 2.4k ops/s | 12.8k ops/s | **26.3k ops/s** | **提升 10.8x** |
| **1.4 MB (15,000 条大数据载荷)** | 160 ops/s | 847 ops/s | **1,818 ops/s** | **提升 11.4x** |

> 详细基准测试配置、硬件环境与完整复现命令请参阅 [docs/benchmarks.md](docs/benchmarks.md)。

---

## 📦 环境准备与快速开始

### 前置要求
- **操作系统**：Linux (推荐 Ubuntu 20.04+)、Windows WSL2、或 macOS。
- **构建工具**：`gcc` 或 `clang`、`make`。
- **Codon 编译器**：安装官方发布的 Codon 编译器：
  ```bash
  curl -fsSL https://exaloop.io/install.sh | bash
  echo 'export PATH="$HOME/.codon/bin:$PATH"' >> ~/.bashrc
  source ~/.bashrc
  ```

### 编译原生 C 加速库
克隆项目源码并编译底层的 `yyjson` C 语言扩展：
```bash
git clone https://github.com/<your-username>/codon_web.git
cd codon_web

# 编译生成 libcfastjson.a 静态库与动态库
make native
```

---

## 💡 快速上手示例

创建示例文件 `main.codon`：

```python
from codon_web import App, Request, Response, CORSMiddleware, TimingMiddleware

# 1. 创建应用实例并开启 OpenAPI / Swagger 交互文档
app = App(
    host="0.0.0.0",
    port=8000,
    title="CodonWeb 企业级 API",
    version="1.0.0",
    docs_url="/docs"
)

# 2. 注册生产级中间件
app.use(CORSMiddleware(allow_origin="*"))
app.use(TimingMiddleware())

# 3. 编写业务路由
@app.get("/")
def home(req: Request) -> Response:
    return Response.json('{"code": 200, "message": "你好，编译型 CodonWeb！"}')

@app.get("/users/{user_id}")
def get_user(req: Request) -> Response:
    user_id = req.param("user_id")
    return Response.json('{"user_id": ' + user_id + ', "status": "active"}')

if __name__ == "__main__":
    app.run()
```

### 编译为机器码并启动服务：
```bash
# 采用 release 优化构建为独立的二进制可执行文件
CODON_PATH=. codon build -release --linker-flags 'src/native/libcfastjson.a' -o app main.codon

# 直接运行二进制服务
./app
```
打开浏览器访问 [http://localhost:8000/docs](http://localhost:8000/docs)，即可直接体验内置的 Swagger UI 交互式接口文档！

---

## 🛠️ 核心功能展示

### 1. 多源参数自动提取与 422 校验
```python
from codon_web import Param, ParamsBinder, Request, Response

item_params = [
    Param.path_int("item_id", description="商品唯一ID"),
    Param.query_str("category", default_value="books", description="商品分类"),
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

### 2. FastAPI 风格依赖注入 (`Depends`)
```python
from codon_web import Request, Response, HTTPBearer, HTTPException

security = HTTPBearer()

def get_current_user(req: Request) -> str:
    token = security(req)
    if token != "valid-secret-token":
        raise HTTPException(401, "无效或过期的 Bearer 凭证")
    return "admin_user"

@app.get("/dashboard")
def dashboard(req: Request) -> Response:
    user = get_current_user(req)
    return Response.json('{"welcome": "' + user + '", "status": "authorized"}')
```

### 3. SIMD 原生 FastJson 极速解析
```python
from codon_web import FastJson

# 零拷贝快速解析
doc = FastJson.loads('{"title": "CodonWeb", "stars": 5000, "features": ["LLVM", "SIMD"]}')
print(doc["title"].as_str())    # "CodonWeb"
print(doc["stars"].as_int())    # 5000

# 极速序列化输出
json_str = doc.dumps()
doc.close()
```

### 4. 高并发异步 MySQL 连接池
```python
from codon_web import AsyncMySQLPool, Request, Response

pool = AsyncMySQLPool(
    host="127.0.0.1", port=3306,
    user="app_user", password="password",
    db="production_db", pool_size=16
)

@app.get("/async/users")
async def fetch_users(req: Request) -> Response:
    conn = await pool.acquire()
    cursor = await conn.cursor()
    await cursor.execute("SELECT id, username, email FROM users LIMIT 10")
    rows = await cursor.fetchall()
    await cursor.close()
    await pool.release(conn)
    return Response.json(rows_to_json(rows))
```

### 5. 内存快速集成测试 (`TestClient`)
```python
from codon_web import TestClient

client = TestClient(app)
res = client.get("/users/10086")

assert res.status_code == 200
assert "active" in res.text
```

---

## 📂 项目模块结构

```
codon_web/
├── codon_web.codon        # 框架顶层公共 API 导出
├── Makefile               # 工程构建、Native C 编译与全量测试管理
├── src/
│   ├── framework/         # 核心框架层：App 引擎、基数树路由、OpenAPI 生成器、依赖注入
│   ├── http/              # HTTP 协议层：快速解析器、请求响应封装、FastJson、Schema 参数校验
│   ├── db/                # 数据库层：MySQL 驱动 (同步/异步)、连接池、SQLite 驱动
│   ├── runtime/           # 运行时层：Linux epoll 反应堆、异步事件循环、上下文
│   ├── ws/                # WebSocket 协议层：RFC 6455 握手、数据帧编解码、哈希校验
│   ├── memory/            # 内存优化层：ByteSlice 零拷贝内存切片封装
│   ├── native/            # 底层 C 加速库：yyjson 源码与 C-ABI 胶水层
│   └── testing/           # 测试套件层：内存级 TestClient 与断言验证
├── examples/              # 完整样例库：FastAPI 范式全功能应用、高并发压测服务
├── tests/                 # 26 套全功能单元与集成测试用例
│   ├── benchmark/         # JSON 与 HTTP 极限性能测试套件
│   └── run_all.codon      # 全量测试一键运行脚本
└── docs/                  # 深度技术文档
    ├── architecture.md    # 核心架构与内部设计原理解析
    └── benchmarks.md      # 性能基准测试与完整复现指南
```

---

## 🧪 自动化测试

运行项目完整测试套件：
```bash
# 1. 编译底层的 native C 库
make native

# 2. 运行核心集成测试
make test

# 3. 运行全部 26 套功能与单元测试
make test-all
```

---

## 🤝 参与贡献

非常欢迎来自社区的 Issue 和 Pull Request！详细贡献规范、开发调试方法与提交格式请参阅 [CONTRIBUTING.md](CONTRIBUTING.md)。

---

## 📄 开源许可证

本项目遵循 [MIT License](LICENSE) 开源许可证。
