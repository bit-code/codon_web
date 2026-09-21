# Contributing to CodonWeb

Thank you for your interest in contributing to **CodonWeb**! We are building the next-generation, native-compiled, ultra-high-performance web framework for the Python ecosystem powered by Codon and LLVM.

---

## Code of Conduct

We are committed to providing a welcoming, inclusive, and harassment-free environment for all contributors. Please be respectful, constructive, and collaborative.

---

## Development Environment Setup

### Prerequisites
- **Operating System**: Linux (Ubuntu 20.04+ recommended) or WSL2 (Windows Subsystem for Linux), or macOS.
- **C Compiler**: `gcc` or `clang` with C99 support and `-fPIC`.
- **Make**: GNU Make 4.0+.
- **Codon Compiler**: Codon 0.16.0+ (installed via `curl -fsSL https://exaloop.io/install.sh | bash` or built from source).
- **Git**: For version control.

### Clone and Build Native Extensions
CodonWeb uses native C acceleration for SIMD-accelerated zero-copy JSON parsing (`yyjson`):

```bash
git clone https://github.com/<your-username>/codon_web.git
cd codon_web

# Compile the native C fast-json engine (libcfastjson.a and libcfastjson.so)
make native
```

---

## Running the Test Suite

CodonWeb comes with a comprehensive test suite covering routing, parsing, middlewares, OpenAPI, async reactor, database drivers, WebSockets, and `TestClient`.

### 1. Run Integration Tests
```bash
make test
```

### 2. Run All 26 Unit and Integration Tests
```bash
CODON_PATH=. codon run --linker-flags 'src/native/libcfastjson.a' tests/run_all.codon
```

### 3. Run JSON Performance Benchmarks
```bash
make bench-json
```

---

## Engineering Guidelines & Standards

All contributions must adhere to clean engineering standards:

1. **Strict Type Safety**: Codon is statically compiled. All function signatures and data structures must have explicit type annotations.
2. **Zero Overhead & Zero Copy**: Prefer slices (`ByteSlice`) and zero-copy references over excessive string copies or heap allocations in the critical HTTP path.
3. **Robust Error Handling**: Never silently swallow exceptions. Use explicit `HTTPException` hierarchy (`BadRequestException`, `NotFoundException`, etc.) and handle socket edge cases.
4. **Clean Code Structure**:
   - Single responsibility per file/class.
   - Keep functions focused and concise.
   - Keep nesting levels $\le 3$.
   - Prefer early returns over deeply nested `if/else`.
5. **No Intermediate Binaries**: Never commit compiled `.o`, `.so`, `.a`, or executable binaries. Keep `.gitignore` intact.

---

## Pull Request Workflow

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
   - `test: add async mysql transaction tests`
4. **Verify tests locally**: Ensure `make test` passes without any errors or warnings.
5. **Push to your fork** and submit a Pull Request to the `main` branch.
6. **Provide a clear description** of the changes, motivation, and any performance impact.
