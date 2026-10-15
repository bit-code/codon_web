# Code-First AOT Schema & Validation Guide

> **Zero-Reflection, Ahead-of-Time Compiled Data Modeling for CodonWeb**

In traditional dynamic web frameworks (such as Pydantic in Python), validating a request body incurs heavy runtime overhead: dynamic introspection, reflection, string allocation, and dict conversions. 

CodonWeb introduces a **Code-First AOT (Ahead-of-Time) Schema Engine** (`src/schema/`). You define schemas using pure, declarative Python syntax. During build time (`make codegen`), CodonWeb parses your model's Abstract Syntax Tree (AST) and generates specialized native serialization, deserialization, and boundary check routines with zero runtime reflection.

---

## 1. Quick Example

Create a schema file, for instance `examples/schemas/user.codon`:

```python
from src.schema import BaseModel, StrField, IntField, FloatField, BoolField, ValidationError

class UserCreate(BaseModel):
    """User registration schema with strict field and domain constraints."""
    username: str = StrField(min_length=3, max_length=32, description="Unique login account name")
    age: int = IntField(ge=0, le=150, default=18, description="User age in years")
    email: str = StrField(format="email", description="Primary contact email address")
    balance: float = FloatField(ge=0.0, le=1000000.0, default=0.0, description="Initial wallet balance")
    is_active: bool = BoolField(default=True, description="Account active flag")
    role: str = StrField(choices=["admin", "member", "guest"], default="member", description="Account permission role")

    def custom_validate(self) -> List[ValidationError]:
        """Cross-field business logic validation hook."""
        errs = List[ValidationError]()
        # Custom business constraint: admin accounts must be at least 21 years old
        if self.role == "admin" and self.age < 21:
            errs.append(ValidationError("age", "Administrator accounts must be at least 21 years old", "value_error.business"))
        return errs
```

---

## 2. Compiling the Schema (`make codegen`)

Run the automated discovery pipeline:

```bash
make codegen
```

The engine automatically:
1. Discovers any `.codon` file with classes inheriting from `BaseModel` or `SchemaModel`.
2. Validates the **Double-Key Verification** invariant (checks that `from src.schema` or `from codon_web` is imported) to eliminate false positives with external ORM models.
3. Computes a checksum of the generated methods.
4. Appends an `@extend class ModelName:` block containing SIMD JSON deserializers, field validators, and formatters directly to the bottom of your file.

Your original class definition at the top of the file remains 100% human-editable!

---

## 3. Supported Field Types & Constraint Options

### 3.1. `StrField`
```python
name: str = StrField(
    min_length=1,           # Minimum string length
    max_length=64,          # Maximum string length
    regex="^[a-zA-Z0-9_]+$",# Regular expression pattern
    format="email",         # Built-in format validator ('email', 'uuid', 'url', 'ipv4')
    choices=["a", "b", "c"],# Whitelist enum values
    default="default_val",  # Default fallback value
    description="Field doc" # OpenAPI metadata description
)
```

### 3.2. `IntField`
```python
age: int = IntField(
    gt=0,                   # Greater than
    ge=18,                  # Greater than or equal to
    lt=100,                 # Less than
    le=99,                  # Less than or equal to
    multiple_of=5,          # Value must be divisible by N
    default=18
)
```

### 3.3. `FloatField`
```python
price: float = FloatField(
    gt=0.0,
    ge=0.01,
    le=99999.99,
    default=0.0
)
```

### 3.4. `BoolField`
```python
is_verified: bool = BoolField(default=False)
```

### 3.5. `ListField` & Nested Models
```python
tags: List[str] = ListField(StrField(min_length=1))
address: AddressSchema = ModelField(AddressSchema)
```

---

## 4. Using Schemas in Route Handlers

CodonWeb integrates seamlessly with the schema engine to provide instant zero-copy request validation:

```python
from codon_web import App, Request, Response
from examples.schemas.user import UserCreate

app = App()

@app.post("/api/users")
def create_user(req: Request) -> Response:
    # 1. Zero-copy SIMD parsing and constraint validation
    res = UserCreate.from_json_doc(req.json_doc())
    
    # 2. Check validation status
    if not res.is_ok():
        # Automatically returns RFC 7807 structured HTTP 422 Unprocessable Entity
        return res.to_response(422)

    # 3. Unwrap the validated, 100% statically-typed native model
    user: UserCreate = res.unwrap()

    # 4. Access fields with native LLVM-inlined dot syntax
    print("Creating user:", user.username, "Age:", user.age)

    # 5. Fast serialization
    return Response.json('{"status": "created", "username": "' + user.username + '"}', 201)
```

---

## 5. Standard RFC 7807 422 Error Responses

When validation fails, `res.to_response(422)` generates an industry-standard error payload:

```json
{
  "detail": [
    {
      "loc": ["username"],
      "msg": "Ensure this value has at least 3 characters",
      "type": "string_too_short"
    },
    {
      "loc": ["age"],
      "msg": "Input should be greater than or equal to 0",
      "type": "greater_than_equal"
    },
    {
      "loc": ["email"],
      "msg": "Value is not a valid email address",
      "type": "value_error.email"
    }
  ]
}
```

---

## 6. Architecture & CodeGen Internals

1. **Zero Runtime Reflection**: Unlike CPython frameworks that iterate over `__annotations__` at runtime, CodonWeb compiles deserialization directly into native machine code.
2. **SIMD Deserialization via FastJson**: The generated `from_json_doc()` accesses raw C JSON values (`yyjson_val`) directly via pointer arithmetic without converting intermediate values to strings.
3. **Double-Key Verification**:
   ```makefile
   AUTO_SCHEMA_SRCS := $(shell find src examples -type f -name "*.codon" 2>/dev/null | \
       grep -v "src/generated" | grep -v "src/schema" | \
       xargs grep -l -E "(SchemaModel|\(BaseModel\))" 2>/dev/null | \
       xargs grep -l -E "(from src\.schema|from codon_web)" 2>/dev/null)
   ```
   This ensures that models defined by external third-party libraries or internal ORMs sharing the name `BaseModel` are never contaminated by the schema generator.
