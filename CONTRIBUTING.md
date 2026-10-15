# Contributing to CodonWeb

Thank you for your interest in contributing to **CodonWeb**! We are building a next-generation, native-compiled, ultra-high-performance web framework and runtime for the Python/Codon ecosystem.

---

## 📜 Code of Conduct

We are committed to providing a welcoming, constructive, and harassment-free community for all participants. Please read and adhere to our [Code of Conduct](CODE_OF_CONDUCT.md).

---

## 🛠️ Development Environment Setup

### 1. Prerequisites
- **Operating System**: Linux (Ubuntu 20.04+ recommended) or Windows WSL2, or macOS.
- **C Compiler**: `gcc` or `clang` with C99 support, `-fPIC`, and POSIX headers.
- **System Libraries**: `make`, `libpq-dev` (PostgreSQL client library), and `python3`.
- **Codon Compiler**: Codon 0.16.0+ (installed via `curl -fsSL https://exaloop.io/install.sh | bash`).
- **Git**: For version control.

### 2. Clone & Compile Native Libraries
CodonWeb uses native C acceleration for SIMD JSON parsing (`yyjson`), PicoHTTPParser, and PostgreSQL drivers:

```bash
git clone https://github.com/bit-code/codon_web.git
cd codon_web

# Compile libcfastjson.a and libcfastjson.so
make native
```

---

## 🧪 Running the Test Suite

CodonWeb includes 33 comprehensive unit and integration test suites covering routing, parsing, middlewares, OpenAPI, async reactor, database drivers, WebSockets, ORM, TaskQ, and `TestClient`.

### 1. Run Core Integration Tests
```bash
make test
```

### 2. Run All 33 Unit & Integration Tests
```bash
make test-all
```

### 3. Run Schema CodeGen & Tests
```bash
make codegen
make test-schema
```

### 4. Run TaskQ Background Queue Tests
```bash
make test-taskq
make test-async-taskq
```

### 5. Run FastJson Benchmarks
```bash
make bench-json
```

---

## 📐 Engineering Guidelines & Standards

All contributions must adhere to clean, production-grade engineering principles:

1. **Strict Type Safety**: Codon is statically compiled. All function signatures and data structures must have explicit, concrete type annotations. Avoid ambiguous dynamic typing.
2. **Zero-Copy & Low-Allocation**: Prefer `ByteSlice` and pointer windows over duplicating byte sequences in critical I/O paths.
3. **No Silent Failures**: Never swallow exceptions. Use the explicit `HTTPException` hierarchy (`BadRequestException`, `NotFoundException`, `UnprocessableEntityException`, etc.) and log meaningful diagnostics.
4. **Code Design & Cleanliness**:
   - Single responsibility per file/class.
   - Keep functions focused and concise (avoid giant functions).
   - Nesting level must be $\le 3$.
   - Prefer early returns over deeply nested `if/else`.
   - Clear and explicit data flow.
5. **No Intermediate Binaries**: Never commit compiled `.o`, `.so`, `.a`, or executable binaries. Keep `.gitignore` intact.

---

## 🧬 Working with the AOT Schema Engine

When adding or modifying schema models in `examples/` or `src/`:
1. Inherit from `BaseModel` or `SchemaModel` from `src.schema` or `codon_web`.
2. Run `make codegen` to update the auto-generated deserialization and validation routines.
3. Verify that `make test-schema` passes.
4. Do not manually edit the code block beneath `# ==================== AUTO-GENERATED METHODS (DO NOT EDIT) ====================`.

---

## 🚀 Pull Request Workflow

1. **Fork the repository** on GitHub.
2. **Create a feature branch**:
   ```bash
   git checkout -b feat/your-feature-name
   # or
   git checkout -b fix/issue-description
   ```
3. **Commit your changes**: Follow the [Conventional Commits](https://www.conventionalcommits.org/) convention:
   - `feat: add multipart streaming parser`
   - `fix: resolve socket timeout edge case in epoll reactor`
   - `perf: optimize radix router prefix lookup`
   - `docs: improve benchmark reproduction guide`
   - `test: add async postgres transaction tests`
4. **Verify tests locally**: Ensure `make test-all` passes without any errors or warnings.
5. **Push to your fork** and open a Pull Request to the `main` branch.
6. **Provide a clear description** of your changes, the rationale behind design decisions, and any performance benchmarks.
