# TaskQ：原生 Codon 分布式后台任务队列详解指南

> **一句话总结**：TaskQ 是为 Codon 静态编译语言量身打造的高性能、无 GIL 限制、纯原生实现的分布式异步任务框架（Celery 对标方案），支持毫秒级任务分发、延时调度、指数退避重试与单 Worker 高并发异步协程。

---

## 一、 通俗理解：TaskQ 是解决什么问题的？

### 1. 传统 Web 模式的痛点（“站在柜台前傻等”）
在传统的 Web 服务中，用户发起一个耗时操作（比如“点击导出 10 万行报表”或“爬取 50 个网页”）：
```text
[用户浏览器] ----发起 HTTP 请求----> [Web 服务器]
   ⏳ (用户浏览器转圈等待 30 秒)       ⚙️ (服务器卡死在生成报表/爬网页)
[用户浏览器] <----收到响应 (超时或慢)--- [Web 服务器]
```
- **问题**：如果此时有 100 个人同时点击导出报表，Web 服务器的线程会被瞬间占满，整个网站直接卡死崩盘。

### 2. TaskQ 的解耦模式（“小票与后厨系统”）
TaskQ 引入了**“生产者 - 消息中介 - 消费者”**的解耦模型：
```text
[用户点击] ---> [Web 服务 (Producer)] 
                     │ 立即生成一张小票 Task ID 并放入队列
                     ▼
             [消息中介 Broker (Redis)]  <--- 类似于“厨房订单架”
                     │ 
                     ▼
             [后台任务 Worker (Consumer)] <--- 独立的后台工人，慢慢炒菜
```
- **流程**：
  1. 用户点击导出，Web 服务器把任务描述打包成一条小消息丢进 Redis，并在 **0.1 毫秒内** 直接给用户返回一个 `task_id`（小票号）；
  2. 用户界面立即恢复响应，可以继续浏览其他页面，或者前端每隔 2 秒拿着 `task_id` 问一下：“我的报表好了没？”；
  3. 后台常驻的 **Worker 进程**（可以和 Web 在同一台机器，也可以部署在其他几十台专用服务器上）自动从 Redis 弹出任务并执行；
  4. 执行完毕后，把报表结果写回 Redis 的“取餐台（Result Backend）”。

---

## 二、 架构全景图

```mermaid
flowchart TD
    subgraph 生产端 [Producers]
        WebAPI["Web 业务进程 (App)"]
        CLI["定时脚本 / 批处理"]
    end

    subgraph 消息中介 [Broker (Redis / Memory)]
        Q_Ready["就绪队列 taskq:queue:default"]
        Q_Delayed["延时集合 taskq:delayed (ZSet by ETA)"]
        Q_DLQ["死信队列 taskq:dead:default (DLQ)"]
        Result_Store["结果后端 taskq:result:task_id (带 TTL)"]
    end

    subgraph 消费端 [Workers]
        SyncW["同步 Worker (适合 CPU 密集计算)"]
        AsyncW["异步 AsyncWorker (适合高并发 I/O 爬虫)"]
    end

    WebAPI -->|client.delay / apply_async| Q_Ready
    WebAPI -->|eta_sec > 0 延时任务| Q_Delayed
    CLI -->|投递任务| Q_Ready

    Q_Delayed -->|到期自动搬迁| Q_Ready
    Q_Ready -->|BRPOP 弹出消费| SyncW
    Q_Ready -->|并发非阻塞 Dequeue| AsyncW

    SyncW -->|重试失败耗尽| Q_DLQ
    AsyncW -->|重试失败耗尽| Q_DLQ

    SyncW -->|写入执行结果| Result_Store
    AsyncW -->|写入执行结果| Result_Store

    WebAPI -.->|async_res.get() 查询结果| Result_Store
```

---

## 三、 两大 Worker 模式：该选哪一个？

TaskQ 根据业务特性的不同，提供了两种专属的 Worker 引擎：

| 维度 | 同步 Worker (`Worker`) | 异步 Worker (`AsyncWorker`) |
| :--- | :--- | :--- |
| **主要定位** | **CPU 密集型任务** | **I/O 密集型高并发任务** |
| **典型场景** | PDF 报表生成、图像/视频处理、大数据矩阵计算 | **网络爬虫**、批量发送 HTTP 请求、短信邮件下发 |
| **任务写法** | 普通函数 `@task("name") def job(p: str):` | 协程类 `class MyTask(AsyncTaskHandler):` |
| **并发机制** | 一个任务执行完再取下一个（或配合多进程/多线程池） | **基于 Epoll 事件循环**，单 Worker 即可支撑成百上千任务并发 |
| **阻塞影响** | 若内部 `time.sleep` 会挂起当前 Worker | 内部 `await sleep` 不会阻塞 Worker，立即切去跑其他任务 |

### 形象比喻：为什么 1 个 AsyncWorker 能并发跑 50 个爬虫？
- **同步 Worker**：像一个老实厨师，煮水时（网络等待）就双手插兜在炉子前站着干等 10 分钟，做完一锅才能做下一锅；
- **异步 AsyncWorker**：像现代大厨，把水烧上（发起异步 HTTP 请求），水没开之前不站着，**立刻转身去点第二锅、第三锅、第五十锅火**！哪个锅的水开了（Epoll 网络数据到达），他就去捞哪个锅的菜！
- **实测性能**：在 `examples/taskq_async_crawler_demo.codon` 中，抓取 30 个需要耗时 80ms 的网页，同步需要 2.4 秒，而 1 个 `AsyncWorker` **仅耗时 0.084 秒（提速 28.5 倍）**！

---

## 四、 手把手实操指南

### 模式一：编写同步后台任务（普通业务/CPU密集型）

#### 1. 定义任务 (`tasks.codon`)
```codon
from src.taskq import task

@task("order:generate_invoice", queue="invoices", max_retries=3)
def generate_invoice(payload: str) -> str:
    # payload 是 JSON 格式的字符串入参
    print(f"Generating invoice for order: {payload}")
    # 模拟生成发票
    return '{"invoice_id": "INV_2026_001", "pdf_url": "https://oss/inv1.pdf"}'
```

#### 2. 生产者投递任务并获取结果
```codon
from src.taskq import TaskClient, RedisBroker

# 1. 建立 Broker 连接
broker = RedisBroker(host="127.0.0.1", port=6379)
client = TaskClient(broker=broker)

# A. 即投即走（不需要关心结果）
task_id = client.delay("order:generate_invoice", '{"order_id": 9527}', queue="invoices")
print("Task queued with ID:", task_id)

# B. 投递并等待结果句柄（AsyncResult）
async_res = client.apply_async("order:generate_invoice", '{"order_id": 9528}', queue="invoices")
result = async_res.get(timeout_sec=5.0) # 阻塞等待最多 5 秒
if result is not None:
    print("Execution status:", result.status) # SUCCESS
    print("Returned data:", result.result)   # {"invoice_id": ...}

# C. 延时任务（10 秒后执行）
client.delay("order:generate_invoice", '{"order_id": 9529}', queue="invoices", eta_sec=10)
```

#### 3. 启动后台 Worker 进程
```codon
from src.taskq import Worker, WorkerConfig, RedisBroker, GLOBAL_TASK_REGISTRY
import tasks # 导入任务定义文件以触发 @task 注册

broker = RedisBroker(host="127.0.0.1", port=6379)
worker = Worker(
    broker=broker,
    registry=GLOBAL_TASK_REGISTRY,
    config=WorkerConfig(queues=["invoices", "default"])
)

print("Worker daemon started...")
worker.start()
```

---

### 模式二：编写异步高并发任务（网络爬虫/批量网络I/O）

#### 1. 定义异步任务 (`crawler_tasks.codon`)
```codon
from src.core.async import sleep
from src.taskq import AsyncTaskHandler

class WebCrawlerTask(AsyncTaskHandler):
    def __init__(self):
        super().__init__(name="crawler:fetch", max_retries=3, default_queue="crawlers")

    async def run(self, payload: str):
        # 模拟非阻塞 HTTP 网络请求（等待远程对端响应）
        await sleep(0.05) 
        return f'{{"url": "{payload}", "status_code": 200, "html_len": 40960}}'
```

#### 2. 启动单 Worker 高并发消费
```codon
from src.core.async import run
from src.taskq import (
    AsyncWorker, 
    AsyncWorkerConfig, 
    AsyncTaskRegistry, 
    RedisBroker
)
import crawler_tasks

broker = RedisBroker(host="127.0.0.1", port=6379)
registry = AsyncTaskRegistry()
registry.register(crawler_tasks.WebCrawlerTask())

# 配置单 Worker 同时维持 100 个并发爬虫任务
config = AsyncWorkerConfig(
    queues=["crawlers"],
    concurrency=100,
    poll_interval_sec=0.01
)

worker = AsyncWorker(broker=broker, registry=registry, config=config)

# 启动非阻塞 Epoll 协程消费引擎
worker.start()
```

---

### 模式三：在 Web RESTful 接口中发起与查询任务 (CodonWeb 生产集成)

在真实的 Web 系统中，HTTP 请求要求在几十毫秒内返回。耗时操作（如爬取网页、发送短信验证码、导出数据）**绝不能在 HTTP Handler 中同步执行**。

完整的 Web API + TaskQ 架构设计模式如下（对应可直接运行的源码 [examples/taskq_web_integration.codon](file:///e:/AiProject/Antigravity/codon_web/examples/taskq_web_integration.codon)）：

```codon
from codon_web import App, Request, Response
from src.taskq import TaskClient, RedisBroker

# 1. 初始化 Broker 与 TaskClient
broker = RedisBroker(host="127.0.0.1", port=6379)
task_client = TaskClient(broker=broker, default_queue="crawlers")

app = App()

# ----------------- 接口 1: 异步提交任务 -----------------
# 客户端发送: POST /api/jobs
# 请求体 JSON: {"url": "https://news.ycombinator.com"}
@app.post("/api/jobs")
def create_crawl_job(req: Request) -> Response:
    body = req.json()
    url = body.get("url", "")
    if len(url) == 0:
        return Response.json('{"error": "Missing \'url\' in request body"}', 400)

    # 1. 微秒级推入 Redis 队列，绝对不阻塞 Web 线程
    task_id = task_client.delay("crawler:fetch", url, queue="crawlers")

    # 2. 遵循标准 REST 规范：返回 202 Accepted 与轮询查询 URL
    resp_body = (
        f'{{"status":"queued","task_id":"{task_id}",'
        f'"check_url":"/api/jobs/{task_id}"}}'
    )
    return Response.json(resp_body, 202)


# ----------------- 接口 2: 轮询查询任务状态与结果 -----------------
# 客户端发送: GET /api/jobs/{task_id}
@app.get("/api/jobs/{task_id:str}")
def get_job_status(req: Request) -> Response:
    task_id = req.param("task_id")
    result = task_client.get_result(task_id)

    if result is None:
        # 任务还在队列排队中，或者 Worker 正在并发抓取中
        return Response.json(f'{{"task_id":"{task_id}","status":"PENDING"}}', 200)

    # 任务已执行完毕 (返回 TaskResult 的完整 JSON，包含 status, result, execution_time_ms)
    return Response.json(result.to_json(), 200)


# ----------------- 接口 3: 延时调度任务 (定时执行) -----------------
# 客户端发送: POST /api/jobs/delayed?delay=15
@app.post("/api/jobs/delayed")
def create_delayed_job(req: Request) -> Response:
    body = req.json()
    url = body.get("url", "")
    delay_sec = req.query_param_int("delay", 10)

    # 投递延时任务 (ETA: 当前时间 + delay_sec 秒后执行)
    task_id = task_client.delay("crawler:fetch", url, queue="crawlers", eta_sec=delay_sec)

    return Response.json(
        f'{{"status":"scheduled","task_id":"{task_id}","eta_seconds":{delay_sec}}}',
        202
    )

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)
```

#### 前端/客户端调用生命周期时序：
1. **客户端发起任务**：`POST /api/jobs`  
   $\rightarrow$ 服务器在 **0.5ms** 内返回 `HTTP 202 Accepted`，响应体包含 `task_id: "tq_1727250000_1"`；
2. **客户端等待/轮询**：每隔 1~2 秒调用 `GET /api/jobs/tq_1727250000_1`：
   - 任务未完成时返回：`{"task_id": "...", "status": "PENDING"}`
   - 任务完成后返回：`{"status": "SUCCESS", "result": "...", "execution_time_ms": 52}`。
3. **彻底解耦**：即使用户断开网络连接或关闭浏览器，后台 Worker 依然会可靠地把爬虫或耗时任务执行完毕并将结果存入 Redis。

---

## 五、 工业级容灾与保障机制

### 1. 指数退避重试 (Exponential Backoff)
当外部接口偶发抖动（如对端报 503 超载）时：
- 第 1 次失败：等待 $2^0 = 1$ 秒后重试；
- 第 2 次失败：等待 $2^1 = 2$ 秒后重试；
- 第 3 次失败：等待 $2^2 = 4$ 秒后重试。
避免所有失败任务瞬间重试打垮下游服务。

### 2. 死信队列 (Dead-Letter Queue, DLQ)
当任务超过设定的 `max_retries` 依然抛出异常时，TaskQ **绝不会丢弃任务，也不会让 Worker 崩溃崩溃**：
- 任务自动归档到对应队列的死信池：`taskq:dead:{queue_name}`；
- 记录详细的最后一次异常调用栈信息；
- 运维人员后续可以通过脚本查看并重新推回队列重新处理。

### 3. 未注册任务保护 (Unregistered Task Safety)
如果上游 Web 代码更新过快，下发了一个未在新版本 Worker 中声明的任务名，Worker 会安全地将其直接隔离至死信队列，主进程正常对外服务。

---

---

## 六、 任务取消机制：工业级多层同步与异步协同取消

在分布式后台任务处理中，“取消（Cancel / Revoke）”绝不是简单地把一个标记位置为 `True`，必须兼顾以下核心工程挑战：
1. **排队中与延时调度任务**：尚未执行时取消，必须立即清除队列，不消耗任何 Worker 算力与并发槽位；
2. **执行前网关校验（Gate Check）**：分布式环境下取消指令与出队可能存在时间差，Worker 必须在真正调用任务逻辑前完成快速失败拦截；
3. **复杂多层嵌套异步任务（Cascading Cancellation）**：现代异步任务（如分布式深度爬虫、多阶段微服务编排）通常包含多层子协程调用。如果只取消父任务，子任务继续在后台空转会造成严重的**幽灵网络请求与内存泄露**。

TaskQ 引入了**树形分层取消令牌（Hierarchical CancellationToken）**与**TaskContext**上下文体系，从根本上解决了复杂多层异步取消问题。

### 1. 核心架构与取消传播链

```mermaid
flowchart TD
    Client["客户端 / Web API"] -->|client.cancel(task_id)| Broker["Broker (Redis / Memory)"]
    Broker -->|写入 taskq:revoked:id 并标记 CANCELLED| RevokedSet["撤销状态存储"]

    subgraph Worker ["Worker / AsyncWorker 引擎"]
        Gate["网关出队检查 is_cancelled(task_id)"]
        ActiveCtx["活动任务上下文 TaskContext"]
        Token["Root CancellationToken"]
        Child1["Child Token (Stage 1)"]
        Child2["Child Token (Stage 2)"]

        Gate -->|已取消 -> 快速终止并记录 CANCELLED| End["立即释放并发槽位"]
        ActiveCtx -->|poll_cancelled 发现远程取消| Token
        Token -->|级联广播取消| Child1
        Token -->|级联广播取消| Child2
    end
```

### 2. 生产级使用示例：多层嵌套异步任务协同取消

```python
from src.taskq import (
    AsyncTaskHandler,
    TaskContext,
    TaskCancelledError
)
from src.core.async import sleep

class DeepCrawlerHandler(AsyncTaskHandler):
    def __init__(self):
        super().__init__(name="deep_crawler")

    async def _level2_fetch_chunk(self, chunk_id: int, ctx: TaskContext):
        for i in range(10):
            # 每步协作式检测取消：一旦检测到取消立即快速失败抛出 TaskCancelledError
            ctx.check_cancelled()
            await sleep(0.05)
        return f"chunk_{chunk_id}_data"

    async def run_with_context(self, payload: str, ctx: TaskContext):
        # 1. 衍生子上下文（父上下文被取消时，所有子上下文自动同步级联取消）
        child_ctx1 = ctx.child_context()
        child_ctx2 = ctx.child_context()

        # 2. 执行第一阶段抓取
        data1 = await self._level2_fetch_chunk(1, child_ctx1)

        # 3. 阶段间协作式检查
        ctx.check_cancelled()

        # 4. 执行第二阶段抓取（若中途用户点击取消，第二阶段绝对不会被执行）
        data2 = await self._level2_fetch_chunk(2, child_ctx2)
        return data1 + " + " + data2
```

### 3. Web API 中发起与取消任务示例

```python
from src.web import App, Request, Response
from src.taskq import TaskClient, RedisBroker

broker = RedisBroker(host="127.0.0.1", port=6379)
client = TaskClient(broker)
app = App()

# 发起耗时任务
@app.post("/api/crawler/start")
def start_crawl(req: Request) -> Response:
    res = client.apply_async("deep_crawler", '{"target": "https://example.com"}')
    return Response.json('{"code": 0, "task_id": "' + res.task_id + '"}')

# 取消任务接口
@app.post("/api/crawler/cancel")
def cancel_crawl(req: Request) -> Response:
    task_id = req.query.get("task_id", "")
    # 无论是排队中还是在执行中的多层异步任务，均可一行代码取消
    ok = client.cancel(task_id)
    return Response.json('{"code": 0, "cancelled": ' + ("true" if ok else "false") + '}')
```

---

## 七、 源码与关键模块索引

| 文件路径 | 职责定位 |
| :--- | :--- |
| [src/taskq/protocol.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/protocol.codon) | 消息协议规范 (`TaskMessage`, `TaskResult`, `TaskCancelledError`, 状态枚举) |
| [src/taskq/context.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/context.codon) | 树形分层取消令牌 (`CancellationToken`) 与任务上下文 (`TaskContext`) |
| [src/taskq/broker/base.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/broker/base.codon) | 中间件抽象接口 (规范入队、阻塞出队、延时调度、死信入队、任务撤销契约) |
| [src/taskq/broker/redis.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/broker/redis.codon) | 原生 Socket 实现的 Redis RESP2 客户端与 Broker (`SETEX revoked`, `EXISTS`) |
| [src/taskq/broker/memory.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/broker/memory.codon) | 纯内存线程/协程级 Broker（支持撤销时就绪队列与延时队列自动物理清除） |
| [src/taskq/registry.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/registry.codon) | 同步任务静态函数指针注册表与 `@task` 装饰器 |
| [src/taskq/async_handler.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/async_handler.codon) | 异步协程任务基类 (`AsyncTaskHandler.run_with_context`) 与注册中心 |
| [src/taskq/worker.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/worker.codon) | 同步任务消费者守护引擎 (网关拦截已撤销任务、多队列优先级、状态监控) |
| [src/taskq/async_worker.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/async_worker.codon) | 异步并发消费者守护引擎 (网关拦截、在途任务状态轮询、级联取消广播) |
| [src/taskq/client.codon](file:///e:/AiProject/Antigravity/codon_web/src/taskq/client.codon) | 统一生产者 API (`delay`, `apply_async`, `cancel`, `AsyncResult`) |

---

## 八、 常见问题答疑 (FAQ)

### Q1：为什么纯 Codon 不能像 Celery 那样直接传任意 Python 对象（如类实例）？
> **解答**：Python 是动态解释型语言，Celery 底层用 `pickle` 序列化内存中的任意 Python 对象，这在工业界存在严重的**安全漏洞**与性能瓶颈。Codon 是严格的编译型语言，所有任务通信一律使用标准的 **JSON 载荷字符串**。这不仅能做到微秒级极速解析，而且保证了微服务接口的清晰和类型安全性。

### Q2：异步任务里可以使用哪些库做 HTTP 请求？
> **解答**：可以直接使用当前项目 [src/os/socket.codon](file:///e:/AiProject/Antigravity/codon_web/src/os/socket.codon) 的原生非阻塞 Socket，或者使用 [src/core/async/stream.codon](file:///e:/AiProject/Antigravity/codon_web/src/core/async/stream.codon) 中的 `open_connection`（已内置基于 Epoll 的 `AsyncStreamReader` 和 `AsyncStreamWriter`）。

### Q3：如何执行现有的测试套件？
在项目根目录下，直接使用预置的 Makefile：
```bash
# 运行基础分布式任务测试套件（覆盖协议、同步执行、重试、延时任务、死信队列等）
make test-taskq

# 运行异步并发任务测试套件（覆盖多协程并发爬虫、限流、异步重试等）
make test-async-taskq

# 运行任务取消测试套件（覆盖同步任务撤销、延时任务清除、异步网关拦截、多层级联取消等）
make test-cancellation

# 运行真实 Redis 全生命周期测试套件（覆盖真实 Redis 连接、同步/异步任务、延时 ZSet、DLQ、真实并发与自动取消）
make test-redis
```
所有测试均在原生编译环境通过，零报错。
