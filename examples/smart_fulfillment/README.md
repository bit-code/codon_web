# 智能电商履约与仓储物流中心 (Smart Fulfillment Hub)

基于 **CodonWeb Code-First AOT 预编译 Schema** 与 **高性能异步 / 同步 ORM** 的工业级微服务参考项目。

本项目达成 **100% 全量覆盖** CodonWeb 的 `schemas` 特性体系与 `orm` 模块特性体系，通过三个紧密协同的电商物流核心业务场景（履约订单、仓储库存、物流轨迹），提供完整的 RESTful 增删改查（CRUD）接口、自动化端到端测试套件，并兼顾同步 ORM 与异步 ORM 的最佳实践。

---

## 1. 架构设计与工程分层

遵循生产级微服务工程化设计，实现领域模型、数据访问、网络传输与配置的分离：

```
examples/smart_fulfillment/
├── README.md                      # 项目设计架构、全功能映射表与接口规范文档
├── config.codon                   # 配置层：履约环境、数据库连接、费用规则计算
├── schemas.codon                  # AOT 契约层：9 个强类型 Schema（覆盖 100% schemas 特性）
├── entities.codon                 # 持久化层：3 张 MySQL 真实数据表（覆盖 100% orm 字段与属性）
├── sync_service.codon             # 同步 ORM 案例层：SyncFulfillmentService（同步 Session CRUD 范式）
├── services.codon                 # 异步 ORM 核心层：FulfillmentService（AsyncMySQLPool + AsyncSession CRUD）
├── api.codon                      # HTTP 控制器层：20 个 RESTful 端点（全场景完整 CRUD 接口）
└── test_fulfillment_app.codon    # 测试验证层：覆盖所有 20 个接口及 12 组工业级业务校验的测试套件
```

---

## 2. 三大业务场景与完整 CRUD 接口矩阵

每一个业务场景均配备规范的 RESTful 增删改查接口，全面驱动异步 ORM (`AsyncSession`) 与 AOT Schemas：

### 场景 1：履约发货订单 (Fulfillment Orders)
- **业务职责**：多商品包裹拆单、时效计费计算、保险费分摊、收件地址与商品清单级联校验。
- **ORM 数据表**：`fulfillment_orders`（覆盖 `IntegerColumn`, `StringColumn`, `FloatColumn`, `BooleanColumn`, `TextColumn`, `SmallIntegerColumn`）。
- **RESTful CRUD 接口**：
  - **Create**: `POST /api/v1/fulfillment/orders`（校验请求契约并异步持久化，返回 201 与自增主键）
  - **Read Single**: `GET /api/v1/fulfillment/orders?tracking_sn=...`（按运单号单点检索并反序列化）
  - **Read List**: `GET /api/v1/fulfillment/orders/list?delivery_speed=...&city=...&limit=...`（基于 SQL Builder 动态多条件过滤检索）
  - **Update**: `PUT /api/v1/fulfillment/orders?tracking_sn=...`（基于 `update(tbl).where().values()` 差异化更新时效、优先级与备注）
  - **Delete**: `DELETE /api/v1/fulfillment/orders?tracking_sn=...`（基于 `delete(tbl).where()` 物理删除并验证 404）

### 场景 2：仓储库存配给 (Warehouse Inventory)
- **业务职责**：大批量 SKU 仓储调度、冷链温控阈值锁定、三维尺寸与多质检项校验、高并发行级锁配给。
- **ORM 数据表**：`warehouse_inventories`（覆盖 `BigIntegerColumn`, `SmallIntegerColumn`, `DecimalColumn(8,3)`, `DateTimeColumn`, `JsonColumn`）。
- **RESTful CRUD 接口**：
  - **Create**: `POST /api/v1/inventory`（校验 `WarehouseInventorySchema` 并持久化大整数与高精度小数）
  - **Read Single**: `GET /api/v1/inventory?sku_id=...`（按 SKU 编号精准检索）
  - **Read List**: `GET /api/v1/inventory/list?warehouse_code=...&min_stock=...`（跨仓库与安全库存范围筛选）
  - **Update**: `PUT /api/v1/inventory?sku_id=...`（更新现有总库存、预留库存、隔离检验状态）
  - **Delete**: `DELETE /api/v1/inventory?sku_id=...`（SKU 下架删除）

### 场景 3：干线物流轨迹 (Shipment Waybill Tracking)
- **业务职责**：干线运单路由追踪、GPS 经纬度数组合规检查、途经中转站子模型路由链路追踪、碳足迹测算。
- **ORM 数据表**：`shipment_trackings`（覆盖 `TinyIntegerColumn`, `DateTimeColumn`, `JsonColumn`, `TextColumn`）。
- **RESTful CRUD 接口**：
  - **Create**: `POST /api/v1/shipments`（录入 `ShipmentTrackingSchema` 包含 GPS 坐标与中转站站点）
  - **Read Single**: `GET /api/v1/shipments?waybill_no=...`（查询物流节点详情）
  - **Read List**: `GET /api/v1/shipments/list?carrier_code=...&status_code=...`（承运商与包裹状态联合检索）
  - **Update**: `PUT /api/v1/shipments?waybill_no=...`（更新包裹最新流转节点与状态备注）
  - **Delete**: `DELETE /api/v1/shipments?waybill_no=...`（运单作废或归档删除）

---

## 3. Schemas 特性全量使用与验证对比

| Schemas 特性模块 | 语法与特性体现 | 落地业务场景与 Schema |
| :--- | :--- | :--- |
| **基础标量约束** | `pattern=r"^SF-[0-9]{8}$"`, `pattern=r"^WB-[A-Z0-9]{10}$"` | `FulfillmentRequest.tracking_sn`, `ShipmentTrackingSchema.waybill_no` |
| **格式与长度校验** | `format="email"`, `min_length=2`, `max_length=120` | `customer_email`, `dispatcher_email`, `AddressSchema` |
| **枚举限定 (choices)** | `choices=["standard", "express", "same_day"]`, `choices=["SF_EXPRESS", ...]` | 配送时效选择、承运商选择、仓库编码选择 |
| **数值边界限制** | `IntField(ge=1, le=999)`, `FloatField(ge=0.01, le=1000.0)` | 仓储分区 `zone_id`、毛重 `gross_weight_kg` |
| **字段别名系统** | `alias="deliverySpeed"`, `alias="zipCode"`, `alias="unitPrice"` | 前端 camelCase 与服务端 snake_case 自动映射互转 |
| **全类型可选字段** | `Optional[str]`, `Optional[int]`, `Optional[float]`, `Optional[bool]` | 备注 `memo`、最低预警线 `min_reorder_level`、温控目标 `target_temperature`、隔离标志 `is_quarantine` |
| **多类型数组与约束** | `ItemsField(0, min_items=3, max_items=3)`<br>`ItemsField(False, min_items=2, max_items=5)`<br>`ItemsField(0.0, min_items=2, max_items=2)` | 三维长宽高 `dimensions_cm: List[int]`<br>质检状态 `inspection_flags: List[bool]`<br>经纬度坐标 `gps_coordinates: List[float]` |
| **单对象嵌套模型** | `address: AddressSchema` | 目的地收件地址嵌套模型 |
| **对象数组嵌套模型** | `items: List[FulfillmentItemSchema]`<br>`route_stations: List[LogisticsRoutePointSchema]` | 包裹明细商品清单、多站点经停中转站路径 |
| **层级错误定位 (loc)** | `["body", "items", "1", "unitPrice"]`, `["body", "gps_coordinates"]` | 嵌套层级与列表索引精确定位回溯 |
| **跨字段风控钩子** | `def custom_validate(self) -> List[ValidationError]` | 当日达城市白名单、总库存不得低于预留库存、温控商品必须提供目标温度、经纬度范围检查 |
| **多源解析与序列化** | `from_json_doc`, `from_dict`, `to_json`, `to_dict`, `write_to_buffer` | 快速 JSON 解析、Query 字典解析、零拷贝直接写内存流 |
| **OpenAPI 3.0 元数据** | `openapi_schema_json()`, `model_json_schema()` | 自动化导出 OpenAPI 3.0 JSON Schema |

---

## 4. ORM 特性全量使用与验证对比

| ORM 特性模块 | 语法与特性体现 | 落地业务场景与实体 / 服务 |
| :--- | :--- | :--- |
| **同步 ORM 会话** | `Session(engine)`, `session.save()`, `session.query()` | `sync_service.codon` 中的 `SyncFulfillmentService` 完整对照案例 |
| **异步 ORM 会话** | `AsyncSession(pool)`, `AsyncMySQLPool`, `async with pool.connection()` | `services.codon` 中的核心异步服务 `FulfillmentService` |
| **全量整型字段** | `IntegerColumn`, `BigIntegerColumn`, `SmallIntegerColumn`, `TinyIntegerColumn` | 主键自增 ID、库存总量 `total_stock`、库区 `zone_id`、物流状态码 `status_code` |
| **浮点与定点数** | `FloatColumn`, `DecimalColumn(precision=8, scale=3)` | 保价金额 `insured_amount`、单件毛重 `unit_weight_kg`（高精度定点数） |
| **文本与大文本** | `StringColumn(max_length, index, unique)`, `TextColumn` | 唯一索引单号、文本明细 `items`、中转路径 `route_stations` |
| **布尔与时间字段** | `BooleanColumn`, `DateTimeColumn(nullable=True)` | 易碎品标记 `fragile`、入库补货时间、发货时间 `dispatched_at` |
| **JSON 列支持** | `JsonColumn("attributes_json")`, `JsonColumn("meta_payload")` | 扩展工业属性与 IoT 传感器元数据结构化存储 |
| **声明式 Base 模型** | `class Model(Base): __tablename__ = tablename(...)` | 继承 `Base`，纯正 SQLAlchemy 风格声明式表定义，零样板 property 冗余 |
| **双向无拷贝下标适配** | `Table(order)`, `order.copy_to(tbl)`, `Schema.from_orm(tbl)` | Schema 与 ORM 实体双向下标直通借用，零临时字典分配、零 GC 损耗 |
| **SQL Expression Builder** | `select().select_from().where().order_by().limit().offset()` | 多条件分页搜索：`list_orders`, `list_inventory`, `list_shipments` |
| **DML 表达式执行** | `update(tbl).where().values()`, `delete(tbl).where()` | 强类型更新与删除语句编译执行 |
| **行级互斥锁 (Row Lock)** | `select().where(...).for_update()` | `lock_and_reserve_stock` 高并发安全锁定库存 |
| **聚合统计** | `session.count(query)` | `count_orders` 订单总量高速统计 |

---

## 5. 编译与一键运行

在项目根目录下执行标准构建命令：

```bash
# 一键触发 AOT 预编译与端到端 20 个接口全场景集成测试
make demo-fulfillment CODON_FLAGS=''
```

测试执行结果：
```
================================================================================
[Smart Fulfillment Hub] Industrial End-to-End AOT Schema & ORM Verification
================================================================================
[OK] Test 0 (Service Healthcheck Endpoint): PASSED
[OK] Test 1 (Full-Featured Valid Request & Financial Metrics): PASSED
[OK] Test 2 (Primitives Validation: Regex, Format, Enum, Bounds): PASSED
[OK] Test 3 (Collection Bounds: List min_items Constraint): PASSED
[OK] Test 4 (Deep Nested Submodels & Hierarchical loc Backtracking): PASSED
[OK] Test 5 (Custom Cross-Field Domain Validation Hooks): PASSED
[OK] Test 6 (Optional[T] Fields, None Mapping & null Parity): PASSED
[OK] Test 7 (Multi-source Ingestion: URL Query Param Coercion): PASSED
[OK] Test 8 (Synchronous ORM Service CRUD Showcase): PASSED
[OK] Test 9 (Zero-Copy ByteBuffer & OpenAPI Spec Introspection): PASSED

--- Testing Scenario 1: Fulfillment Orders Full CRUD Endpoints ---
  [CRUD 1/4] POST /api/v1/fulfillment/orders (Create): PASSED
  [CRUD 2/4] GET /api/v1/fulfillment/orders (Read Single): PASSED
  [CRUD 2b] GET /api/v1/fulfillment/orders/list (Filtered Search): PASSED
  [CRUD 3/4] PUT /api/v1/fulfillment/orders (Update): PASSED
  [CRUD 4/4] DELETE /api/v1/fulfillment/orders (Delete & Verify 404): PASSED

--- Testing Scenario 2: Warehouse Inventory Full CRUD Endpoints ---
  [CRUD 1/4] POST /api/v1/inventory (Create Inventory): PASSED
  [CRUD 2/4] GET /api/v1/inventory (Read Single): PASSED
  [CRUD 2b] GET /api/v1/inventory/list (Filtered Search): PASSED
  [CRUD 3/4] PUT /api/v1/inventory (Update Stock): PASSED
  [CRUD 4/4] DELETE /api/v1/inventory (Delete & Verify 404): PASSED
  [Validation] Inventory stock consistency constraint: PASSED

--- Testing Scenario 3: Shipment Tracking Full CRUD Endpoints ---
  [CRUD 1/4] POST /api/v1/shipments (Create Waybill): PASSED
  [CRUD 2/4] GET /api/v1/shipments (Read Waybill): PASSED
  [CRUD 2b] GET /api/v1/shipments/list (Filtered Search): PASSED
  [CRUD 3/4] PUT /api/v1/shipments (Update Waybill Status): PASSED
  [CRUD 4/4] DELETE /api/v1/shipments (Delete & Verify 404): PASSED
  [Validation] Shipment GPS boundary cross-field validation: PASSED

================================================================================
ALL 12 Industrial Schema & Async ORM Full CRUD Verification Scenarios PASSED!
================================================================================
```
