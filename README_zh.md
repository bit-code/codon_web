# CodonWeb

<p align="center">
  <strong style="font-size: 1.4em;">基于 Codon (LLVM) 的下一代原生编译型超高性能 Web 框架与运行时</strong>
</p>

<p align="center">
  <a href="https://github.com/bit-code/codon_web/actions"><img src="https://img.shields.io/badge/CI-%E6%9E%84%E5%BB%BA%E9%80%9A%E8%BF%87-brightgreen?style=flat-square&logo=githubactions" alt="CI Status"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/%E5%BC%80%E6%BA%90%E5%8D%8F%E8%AE%AE-MIT-blue?style=flat-square" alt="License"></a>
  <a href="https://github.com/exaloop/codon"><img src="https://img.shields.io/badge/%E7%BC%96%E8%AF%91%E5%99%A8-Codon%20(LLVM)-blueviolet?style=flat-square" alt="Codon"></a>
  <img src="https://img.shields.io/badge/%E5%90%9E%E5%90%90%E9%87%8F-425k%2B%20QPS-orange?style=flat-square" alt="Throughput">
  <img src="https://img.shields.io/badge/%E5%86%85%E5%AD%98%E5%B8%B8%E9%A9%BB-~18MB%20RSS-green?style=flat-square" alt="Memory">
  <a href="README.md"><img src="https://img.shields.io/badge/docs-English-blue?style=flat-square" alt="English Documentation"></a>
</p>

<p align="center">
  <a href="README.md">English</a> | <a href="README_zh.md">简体中文</a>
</p>

---

## ⚡ 什么是 CodonWeb？

**CodonWeb** 是为 [Codon](https://github.com/exaloop/codon) 编译器量身定制的下一代原生编译型 Web 框架与高性能异步运行时。它直接将优雅、地道的 Pythonic 代码通过 LLVM 静态预编译（AOT）为独立的机器码二进制文件。

通过彻底打破 Python 全局解释器锁（GIL）、消除动态反射与解释器开销，CodonWeb 在保持 **FastAPI 与 SQLAlchemy 级极致开发体验** 的同时，释放了媲美 **C++ 与 Rust（Actix-Web）的原生吞吐与毫秒级低延迟**。

```
              ┌─────────────────────────────────────────────────────────────┐
              │                   FastAPI 风格极简声明语法                   │
              │         (路由装饰器、类型提示、Depends 依赖注入、OpenAPI)        │
              └──────────────────────────────┬──────────────────────────────┘
                                             │ AOT 静态预编译
                                             ▼
              ┌─────────────────────────────────────────────────────────────┐
              │                 Codon + LLVM 深度编译优化                   │
              │         (独立机器码、函数极致内联、彻底无 GIL 限制)             │
              └──────────────────────────────┬──────────────────────────────┘
                                             │ 直通 Linux 内核
                                             ▼
              ┌─────────────────────────────────────────────────────────────┐
              │                Rust / C++ 级极限执行性能                     │
              │        (425,000+ QPS, 0.7ms 延迟, 仅 ~18MB 内存常驻)          │
              └─────────────────────────────────────────────────────────────┘
```

---

## 🚀 核心亮点

- 🏎️ **机器级原生吞吐**：在普通多核硬件下，吞吐量突破 **425,000+ QPS**，P50 延迟低于 **0.78 毫秒**。
- 🧬 **Code-First AOT Schema 校验**：声明式 `BaseModel` 在构建期直接生成 SIMD 反序列化与边界校验机器码，彻底告别运行时反射性能惩罚。
- ⚡ **SIMD FastJson (`yyjson`)**：直通 C-ABI，以 **2.4 ~ 3.1 GB/s** 的极速吞吐处理 JSON 序列化（比 Python `json` 快 12 倍，比 `orjson` 快 2 倍）。
- 🗄️ **工业级 ORM 与 SQL 构造器**：支持 MySQL 与 PostgreSQL 双方言，具备 SQLAlchemy 风格流畅构造语法（`select().where().order_by()`）与异步非阻塞连接池。
- 📦 **TaskQ 分布式后台任务队列**：对标 Celery 的原生异步任务系统，内置 Redis Broker、延时调度（ZSet）、指数退避重试，以及支持海量并发的协程 Worker（`AsyncWorker`）。
- 🔄 **统一单事件循环（USEL）**：整合 Linux Epoll 与 `io_uring` 反应堆，基于 `ByteSlice` 实现 TCP 网络缓冲区的零拷贝切片。
- 🛡️ **生产级洋葱中间件**：提供 CORS、滑动窗口/独立 IP 限流、HMAC-SHA256 加密 Cookie 会话、安全响应头与 Panic 崩溃恢复。
- 💬 **全功能实时协议**：完整实现 RFC 6455 WebSocket、Server-Sent Events（SSE）与 HTTP/1.1 分块传输流。
- 📜 **全自动 OpenAPI 3.0**：内置交互式 Swagger UI（`/docs`）与 ReDoc（`/redoc`）接口文档生成。
- 🧪 **零端口内存测试客户端（`TestClient`）**：无需绑定物理网络套接字，在毫秒内完成全链路端到端集成测试。

---

## 📊 性能实测基准对比

### 1. HTTP 服务吞吐量对比 (`wrk` 400 并发, 12 线程)

| 框架 | 语言与运行时 | 每秒请求数 (QPS) | P50 延迟 | P99 延迟 | 内存常驻 (RSS) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **CodonWeb** | **Codon (LLVM 原生机器码)** | **455,320** | **0.78 ms** | **1.82 ms** | **~18 MB** |
| **Actix-Web** | Rust (原生 + jemalloc) | **462,110** | **0.71 ms** | **1.65 ms** | **~24 MB** |
| **Gin** | Go 1.22 (Goroutine) | **264,800** | **1.42 ms** | **3.50 ms** | **~38 MB** |
| **Fastify** | Node.js 20 (V8 JIT) | **86,400** | **4.20 ms** | **9.80 ms** | **~95 MB** |
| **FastAPI** | Python 3.12 (Uvicorn + uvloop) | **19,250** | **20.40 ms** | **46.80 ms** | **~122 MB** |

### 2. JSON 序列化与反序列化吞吐 (ops/sec)

| 测试载荷规模 | Python 标准库 `json` | Python `orjson` | **Codon FastJson (`yyjson`)** | 性能加速比 |
| :--- | :--- | :--- | :--- | :--- |
| **API 单对象 (0.4 KB)** | 520k ops/s | 2,630k ops/s | **6,670k ops/s** | **提升 12.8x** |
| **批量记录 (92 KB)** | 2,440 ops/s | 12,800 ops/s | **26,300 ops/s** | **提升 10.8x** |
| **海量报表 (1.4 MB)** | 160 ops/s | 847 ops/s | **1,818 ops/s** | **提升 11.4x** |

*关于详细测试环境、脚本与完整复现指引，请参阅 [docs/benchmarks.md](docs/benchmarks.md)。*

---

## 📦 环境准备与快速上手

### 1. 前置依赖要求
- **操作系统**：Linux（推荐 Ubuntu 20.04+）、Windows WSL2、或 macOS。
- **构建工具**：`gcc` 或 `clang`、`make`、`libpq-dev`。
- **Codon 编译器**：
  ```bash
  curl -fsSL https://exaloop.io/install.sh | bash
  echo 'export PATH="$HOME/.codon/bin:$PATH"' >> ~/.bashrc
  source ~/.bashrc
  ```

### 2. 获取源码与编译 C 语言底层扩展
```bash
git clone https://github.com/bit-code/codon_web.git
cd codon_web

# 编译 C 语言 SIMD 极速扩展库（libcfastjson.a、picohttpparser、libpq 桥接）
make native
```

### 3. 创建你的第一个应用 (`main.codon`)
```python
from codon_web import App, Request, Response, CORSMiddleware, TimingMiddleware

# 1. 创建应用实例，开启交互式接口文档
app = App(
    host="0.0.0.0",
    port=8000,
    title="CodonWeb API",
    version="1.0.0",
    docs_url="/docs"
)

# 2. 注册中间件
app.use(CORSMiddleware(allow_origin="*"))
app.use(TimingMiddleware())

# 3. 编写业务路由
@app.get("/")
def home(req: Request) -> Response:
    return Response.json('{"message": "你好，原生编译型 CodonWeb！"}')

@app.get("/users/{user_id}")
def get_user(req: Request) -> Response:
    user_id = req.param("user_id")
    return Response.json('{"id": ' + user_id + ', "status": "active"}')

if __name__ == "__main__":
    app.run()
```

### 4. 静态编译与启动
```bash
# 静态编译为独立的 release 优化二进制机器码
CODON_PATH=. codon build -release --linker-flags 'src/native/libcfastjson.a -lpq' -o app main.codon

# 启动独立二进制服务
./app
```
打开浏览器访问 [http://localhost:8000/docs](http://localhost:8000/docs) 即可体验 Swagger UI 交互式文档！

---

## 🛠️ 核心功能展示

### 1. Code-First AOT Schema 声明式校验
使用地道的 Python 类声明实体：
```python
from src.schema import BaseModel, StrField, IntField, FloatField

class UserCreate(BaseModel):
    username: str = StrField(min_length=3, max_length=32)
    age: int = IntField(ge=18, le=120)
    email: str = StrField(format="email")
    balance: float = FloatField(ge=0.0, default=0.0)
```
执行 `make codegen` 自动生成零反射机器码。在路由中直接消费：
```python
@app.post("/api/users")
def register_user(req: Request) -> Response:
    res = UserCreate.from_json_doc(req.json_doc())
    if not res.is_ok():
        return res.to_response(422)  # 自动输出 RFC 7807 结构化 422 错误
    
    user = res.unwrap()
    return Response.json('{"created": "' + user.username + '"}', 201)
```

### 2. SQLAlchemy 风格流畅 SQL 构造器
兼具编译期类型安全与多数据库方言无缝适配：
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

# 自动编译为目标数据库 SQL：MySQL（反引号、?）或 PostgreSQL（双引号、$1）
sql, params = query.to_sql(MYSQL_DIALECT)
```

### 3. TaskQ 分布式后台任务系统
将耗时或高并发网络任务轻松剥离至后台进程异步处理：
```python
from codon_web import TaskClient
from src.taskq.broker.redis import RedisBroker

broker = RedisBroker("127.0.0.1", 6379)
client = TaskClient(broker)

# 0.1 毫秒内快速投递任务并获取小票号
async_res = client.delay("generate_report", '{"user_id": 10086}')
print("已派发任务 ID:", async_res.task_id)
```
*单台 `AsyncWorker` 基于协程并发，比普通同步 Worker **处理高并发网络爬虫快 28 倍**（详见 [docs/taskq.md](docs/taskq.md)）。*

### 4. FastAPI 风格依赖注入 (`Depends`)
```python
from codon_web import Request, HTTPBearer, HTTPException

security = HTTPBearer()

def get_current_user(req: Request) -> str:
    token = security(req)
    if token != "secret-token":
        raise HTTPException(401, "无效或已过期的访问凭证")
    return "authorized_user"

@app.get("/api/profile")
def profile(req: Request) -> Response:
    user = get_current_user(req)
    return Response.json('{"user": "' + user + '"}')
```

### 5. RFC 6455 完整 WebSocket 协议支持
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
            conn.send_text("回声响应: " + frame.payload)
```

### 6. 零网络开销极速内存测试 (`TestClient`)
```python
from codon_web import TestClient

client = TestClient(app)
res = client.get("/users/123")

assert res.status_code == 200
assert "status" in res.text
```

---

## 📂 项目模块结构

```
codon_web/
├── codon_web.codon             # 框架顶层公共 API 统一导出
├── Makefile                    # 构建、C 语言扩展编译与全量测试管理
├── src/
│   ├── framework/              # App 引擎、基数树路由、OpenAPI 生成器、依赖注入
│   ├── http/                   # PicoHTTPParser、请求/响应、FastJson、流式 SSE
│   ├── schema/                 # Code-First AOT Schema 体系、AST 校验器、422 错误
│   ├── orm/                    # 多方言（MySQL/PostgreSQL）、模型、SQLAlchemy 风格 SQL 构造器
│   ├── db/                     # 原生 MySQL/PostgreSQL 异步连接池、SQLite、Redis 驱动
│   ├── taskq/                  # 原生分布式任务队列（同步 Worker 与协程 AsyncWorker）
│   ├── runtime/                # Linux Epoll 反应堆、USEL 事件循环、io_uring、时间缓存
│   ├── ws/                     # RFC 6455 WebSocket 协议（帧编解码、握手、加密）
│   ├── memory/                 # ByteSlice 零拷贝内存切片抽象
│   ├── native/                 # C-ABI 极速扩展：yyjson、picohttpparser、libpq、io_uring
│   └── testing/                # 内存级 TestClient 与断言测试框架
├── tools/                      # AOT Schema 预编译代码生成器（AST + Jinja2）
├── benchmarks/                 # CodonWeb vs Actix-Web (Rust) 吞吐量对比压测套件
├── examples/                   # 生产级示例库（FastAPI 范式、AOT Schema、TaskQ 爬虫）
├── tests/                      # 33 套完整的单元与集成测试套件
└── docs/                       # 深度技术与架构文档
    ├── architecture.md         # 核心架构解析与零拷贝内存模型
    ├── benchmarks.md           # 性能基准实测与完整复现指引
    ├── schema_guide.md         # Code-First AOT Schema 指南与边界校验
    ├── orm_guide.md            # ORM、Session 生命周期与流畅 SQL 构造器
    ├── taskq.md                # TaskQ 分布式队列架构与实操指南
    └── deployment.md           # 生产级部署、Systemd、Nginx 与 Docker 容器化
```

---

## 📖 深度技术文档

- 🏛️ [核心架构设计与零拷贝内存模型](docs/architecture.md)
- 📊 [极限性能基准实测与复现指南](docs/benchmarks.md)
- 🧬 [Code-First AOT Schema 完整开发指南](docs/schema_guide.md)
- 🗄️ [工业级 ORM 与流畅 SQL 构造器指南](docs/orm_guide.md)
- 📦 [TaskQ 原生分布式任务队列实操指南](docs/taskq.md)
- 🚢 [生产级部署、Systemd、Nginx 与 Docker 指南](docs/deployment.md)

---

## 🧪 自动化测试

运行全部 33 套自动化单元与集成测试：
```bash
# 1. 编译底层的 native C 库
make native

# 2. 运行核心集成测试
make test

# 3. 运行全量 33 套单元与集成测试
make test-all

# 4. 运行 AOT Schema 校验测试
make test-schema

# 5. 运行 TaskQ 分布式任务队列测试
make test-taskq
```

---

## 🤝 参与贡献

非常欢迎来自开源社区的 Issue 与 Pull Request！提交前请阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 了解代码风格规范、本地调试方法与 PR 提交流程。

---

## 📄 开源许可证

本项目遵循 [MIT License](LICENSE) 开源许可证。
