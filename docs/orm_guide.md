# CodonWeb ORM & SQL Builder Guide

> **Type-Safe, Multi-Dialect Object-Relational Mapping & Fluent SQL Query Engine**

CodonWeb features an industrial-grade ORM and SQL Expression Engine (`src/orm/`) tailored specifically for native ahead-of-time compiled environments. It combines the declarative ergonomics of SQLAlchemy and Django ORM with native machine code performance and zero reflection overhead.

---

## 1. Key Capabilities

- **Multi-Dialect Support**: Full compatibility with **MySQL 5.7+ / 8.0+** and **PostgreSQL 12+**.
- **Dual Connection Modes**: Synchronous (`Session`) and fully non-blocking asynchronous (`AsyncSession`) operation modes.
- **SQLAlchemy-Style Expression Builder**: Express complex SQL queries with type-safe, fluent method chaining (`select().where().order_by().limit()`).
- **Automatic Dialect Compilation**: Automatically handles database-specific identifier quoting (backticks vs double quotes) and parameter placeholders (`?` vs `$1, $2`).
- **Connection Pool Integration**: Direct connection pooling with `AsyncMySQLPool` and `AsyncPostgresPool`.

---

## 2. Defining Declarative Models

Models are defined by inheriting from `Model` and providing schema metadata via `ModelMeta`:

```python
from typing import List, Optional
from codon_web import (
    Model, ModelMeta, Field,
    IntegerField, CharField, BooleanField, FloatField, DateTimeField
)

class User(Model):
    def __init__(self):
        super().__init__()

    @staticmethod
    def get_meta() -> ModelMeta:
        fields = List[Field]()
        fields.append(IntegerField("id", primary_key=True, auto_increment=True))
        fields.append(CharField("username", max_length=64, unique=True, nullable=False))
        fields.append(CharField("email", max_length=128, nullable=False))
        fields.append(IntegerField("age", default_val="18"))
        fields.append(FloatField("balance", default_val="0.0"))
        fields.append(BooleanField("is_active", default_val="1"))
        fields.append(DateTimeField("created_at", auto_now_add=True))
        return ModelMeta("users", fields)
```

### Supported Column Types
| Field Class | MySQL Type | PostgreSQL Type | Python/Codon Type |
| :--- | :--- | :--- | :--- |
| `IntegerField` | `INT` | `INTEGER` | `int` |
| `BigIntegerField` | `BIGINT` | `BIGINT` | `int` |
| `SmallIntegerField` | `SMALLINT` | `SMALLINT` | `int` |
| `CharField` | `VARCHAR(N)` | `VARCHAR(N)` | `str` |
| `TextField` | `TEXT` | `TEXT` | `str` |
| `FloatField` | `FLOAT` | `REAL` | `float` |
| `DoubleField` | `DOUBLE` | `DOUBLE PRECISION`| `float` |
| `BooleanField` | `TINYINT(1)` | `BOOLEAN` | `bool` |
| `DateTimeField` | `DATETIME` | `TIMESTAMP` | `str` |

---

## 3. Database Engine & Connection URLs

Initialize the `DatabaseEngine` using standard database connection strings:

```python
from codon_web import DatabaseEngine

# MySQL Connection Engine with pool bounds
engine_mysql = DatabaseEngine.from_url(
    "mysql://root:password@127.0.0.1:3306/production_db",
    min_pool_size=4,
    max_pool_size=20
)

# PostgreSQL Connection Engine
engine_pg = DatabaseEngine.from_url(
    "postgresql://postgres:secret@127.0.0.1:5432/production_db",
    min_pool_size=4,
    max_pool_size=20
)
```

---

## 4. SQLAlchemy-Style Fluent SQL Query Builder

The SQL expression engine (`src/orm/sql.codon`) allows you to compose queries cleanly without raw string concatenation or SQL injection vulnerabilities:

```python
from codon_web.orm import Table, select, and_, or_, not_, desc, asc, func

# 1. Define Table abstraction
users = Table("users")

# 2. Build declarative query
query = (
    select(users.id, users.username, users.email, users.balance)
    .select_from(users)
    .where(
        users.is_active == True,
        users.age >= 18,
        users.balance > 100.0
    )
    .order_by(users.id.desc())
    .limit(20)
    .offset(0)
)
```

### Multi-Dialect Compilation
Compile the expression for different database engines:

#### For MySQL:
```python
from codon_web.orm import MYSQL_DIALECT

sql, params = query.to_sql(MYSQL_DIALECT)
# Compiled SQL:
# SELECT `users`.`id`, `users`.`username`, `users`.`email`, `users`.`balance` 
# FROM `users` 
# WHERE `users`.`is_active` = ? AND `users`.`age` >= ? AND `users`.`balance` > ? 
# ORDER BY `users`.`id` DESC LIMIT 20 OFFSET 0
```

#### For PostgreSQL:
```python
from codon_web.orm import POSTGRESQL_DIALECT

sql, params = query.to_sql(POSTGRESQL_DIALECT)
# Compiled SQL:
# SELECT "users"."id", "users"."username", "users"."email", "users"."balance" 
# FROM "users" 
# WHERE "users"."is_active" = $1 AND "users"."age" >= $2 AND "users"."balance" > $3 
# ORDER BY "users"."id" DESC LIMIT 20 OFFSET 0
```

---

## 5. Advanced Query Expressions

### Logical Combinators (`and_`, `or_`, `not_`)
```python
condition = and_(
    users.age >= 21,
    or_(
        users.role == "admin",
        users.role == "moderator"
    ),
    not_(users.is_banned == True)
)

stmt = select(users.id, users.username).select_from(users).where(condition)
```

### In-Clause & Null Checks
```python
# IN clause
stmt = select(users.id).select_from(users).where(users.status.in_(["ACTIVE", "PENDING"]))

# NULL checks
stmt = select(users.id).select_from(users).where(users.deleted_at.is_null())
```

### Aggregations (`func.count`, `func.max`, `func.avg`)
```python
stmt = (
    select(func.count(users.id), func.avg(users.balance))
    .select_from(users)
    .where(users.is_active == True)
)
```

### Insert, Update & Delete Expressions
```python
from codon_web.orm import insert, update, delete

# Insert Statement
ins = (
    insert(users)
    .values({"username": "alice", "email": "alice@example.com", "age": 25})
)

# Update Statement
upd = (
    update(users)
    .where(users.id == 101)
    .values({"balance": 500.0, "is_active": True})
)

# Delete Statement
del_stmt = delete(users).where(users.id == 101)
```

---

## 6. End-to-End Session & Async Operations

### Asynchronous Route Integration
```python
from codon_web import App, Request, Response, AsyncMySQLPool

app = App()
pool = AsyncMySQLPool("127.0.0.1", 3306, "app_user", "password", "app_db", pool_size=16)

@app.get("/api/users")
async def list_users(req: Request) -> Response:
    async with pool.connection() as conn:
        cursor = conn.cursor()
        
        # Build query
        table = Table("users")
        stmt = select(table.id, table.username, table.email).select_from(table).where(table.is_active == True).limit(50)
        sql, params = stmt.to_sql(MYSQL_DIALECT)
        
        await cursor.execute(sql, params)
        rows = await cursor.fetchall()
        
        return Response.json(rows_to_json(rows))
```
