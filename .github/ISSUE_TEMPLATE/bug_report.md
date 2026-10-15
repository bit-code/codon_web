---
name: Bug Report
about: Create a report to help us improve CodonWeb
title: '[BUG] '
labels: ['bug', 'triage']
assignees: ''
---

**Describe the Bug**
A clear and concise description of what the bug is.

**Minimal Reproducible Example**
Please provide a self-contained `.codon` code snippet that triggers the issue:

```python
from codon_web import App, Request, Response

app = App()

# Reproducible route or handler here
```

**Steps to Reproduce**
1. Compile with: `CODON_PATH=. codon build -release ...`
2. Run executable: `./app`
3. Send request: `curl ...`
4. See error: `...`

**Expected Behavior**
A clear and concise description of what you expected to happen.

**Actual Output / Error Log**
```text
Paste terminal output or stack trace here
```

**Environment (please complete the following information):**
- OS: [e.g. Ubuntu 22.04 LTS, Windows WSL2, macOS]
- Kernel Version: [e.g. Linux 6.5.0]
- Codon Version: [e.g. `codon --version`]
- GCC / Clang Version: [e.g. `gcc --version`]
- CodonWeb Git Commit: [e.g. `git rev-parse HEAD`]

**Additional Context**
Add any other context about the problem here (e.g., whether this happens under high concurrency or specific payloads).
