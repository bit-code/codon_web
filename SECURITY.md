# Security Policy

## Supported Versions

We provide security updates and patches for the following versions of CodonWeb:

| Version | Supported          |
| ------- | ------------------ |
| 0.2.x   | :white_check_mark: |
| 0.1.x   | :x:                |

---

## Reporting a Vulnerability

We take the security of CodonWeb seriously. If you discover a security vulnerability or potential threat within the framework (such as memory safety vulnerabilities in native C bindings, HTTP request smuggling, path traversal, authentication bypass, or denial of service):

1. **Do not create a public GitHub issue.**
2. Send a detailed report privately to the core security team or through GitHub's [Private Vulnerability Reporting](https://github.com/bit-code/codon_web/security/advisories/new).
3. Include the following details in your report:
   - A description of the vulnerability and its potential impact.
   - Exact steps or minimal reproducible code (`.codon` test case) to trigger the behavior.
   - The environment details (operating system, kernel version, Codon compiler version).
   - Any suggested mitigations or patches if available.

### Response Timeline
- **Initial Acknowledgment**: Within 48 hours of receipt.
- **Triage & Assessment**: Within 5 business days.
- **Patch Release & Disclosure**: Coordinated disclosure after a fix has been verified and released.

Thank you for helping keep CodonWeb safe and reliable for everyone.
