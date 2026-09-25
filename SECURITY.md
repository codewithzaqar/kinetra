# Security Policy

## Supported Versions

We release patches for security vulnerabilities. Currently, the following versions are supported:

| Version | Supported          |
| ------- | ------------------ |
| 0.0.x   | :white_check_mark: |
| < 0.0.1 | :x:                |

## What Constitutes a Security Vulnerability?

Kinetra is a C-based runtime that executes untrusted scripts. Because of this, the line between a "language bug" and a "security vulnerability" is important:

**Security Vulnerabilities (Please report privately):**
* **Host Crashes / Segfaults:** A Kinetra script that causes the C runtime to segfault or abort unexpectedly.
* **Memory Corruption:** Buffer overflows in the lexer, parser, or VM that could lead to arbitrary code execution.
* **Sandbox Escapes:** (If/when a sandbox is implemented) Any way for a script to access the host filesystem or network.

**Standard Bugs (Please open a public GitHub Issue):**
* **Infinite Loops / High CPU:** A script that hangs the VM (this is expected behavior for a Turing-complete language without execution timeouts).
* **Memory Leaks:** Scripts that consume too much memory over time (Kinetra currently lacks a Garbage Collector; memory grows until process exit).
* **Incorrect Math / Logic:** The VM producing the wrong output for a valid script.

## Reporting a Vulnerability

**Please do NOT report security vulnerabilities through public GitHub issues.**

If you believe you have found a security vulnerability in Kinetra, please report it to us using one of the following methods:

### Option 1: GitHub Private Vulnerability Reporting (Preferred)
1. Navigate to the **Security** tab of this repository.
2. Click **"Report a vulnerability"**.
3. Fill out the form with details about the vulnerability and the Kinetra script that triggers it.

### Option 2: Email
Send an email to **[INSERT YOUR SECURITY EMAIL ADDRESS]**. 
* Include the word "SECURITY" in the subject line.
* Provide a minimal Kinetra script (`.knt`) that reproduces the crash or exploit.
* Include your OS, compiler version (e.g., GCC/Clang), and whether you were using the tree-walk VM or the bytecode VM (`--bc`).

## What to Expect

1. **Acknowledgment:** We will acknowledge receipt of your report within **48 hours**.
2. **Assessment:** We will investigate the issue, determine its severity, and identify the affected versions.
3. **Resolution:** We will work on a patch and prepare a security release. We will keep you updated on our progress.
4. **Disclosure:** Once the patch is released, we will coordinate with you to publicly disclose the vulnerability (usually via a GitHub Security Advisory and a `CHANGELOG.md` entry), giving you full credit for the discovery unless you prefer to remain anonymous.

Thank you for helping keep Kinetra and its users safe!