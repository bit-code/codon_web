## Description
Briefly describe what this PR does, why it is needed, and link any related issues (e.g., `Fixes #123`).

## Type of Change
- [ ] 🐛 Bug fix (non-breaking change which fixes an issue)
- [ ] 🚀 New feature (non-breaking change which adds functionality)
- [ ] ⚡ Performance optimization (latency reduction, zero-copy enhancement)
- [ ] 💥 Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] 📖 Documentation update
- [ ] 🧪 Tests / CI update

## Architectural & Performance Considerations
- [ ] Preserves zero-copy slicing (`ByteSlice`) and avoids unnecessary heap allocations in the hot path.
- [ ] All functions and data structures have strict type annotations.
- [ ] Error handling uses explicit exceptions without silently catching panics.

## Verification & Testing
Describe how you tested these changes:
- [ ] `make native` succeeds.
- [ ] `make test` passes.
- [ ] `make test-all` passes (all 33 unit and integration suites).
- [ ] Added new tests in `tests/` covering the new behavior.

## Checklist:
- [ ] My code follows the code style guidelines of this project (see [CONTRIBUTING.md](CONTRIBUTING.md)).
- [ ] I have performed a self-review of my own code.
- [ ] I have commented my code, particularly in hard-to-understand areas or C-ABI bindings.
- [ ] I have updated the documentation accordingly.
- [ ] No intermediate binary files (`.o`, `.a`, `.so`, executables) have been committed.
