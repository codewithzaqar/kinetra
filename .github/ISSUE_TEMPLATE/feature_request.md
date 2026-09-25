---
name: Feature Request
about: Propose new syntax, a type, a built-in, or tooling for Kinetra
title: "[Feature] "
labels: enhancement
assignees: ''
---

## Summary
One paragraph describing the feature.

## Motivation
What problem does it solve? Which simulations or programs become easier
or faster?

## Proposed syntax / semantics
Example Kinetra code showing intended usage and behavior:

```kinetra
// proposed usage
```

- Precedence / scoping / typing rules (if applicable):
- Interaction with existing features (`sim`, `parallel for`, `const`, ...):

## Affected components
- [ ] Lexer
- [ ] Parser
- [ ] Tree-walk VM
- [ ] Bytecode VM (parity, or intentional codegen error)
- [ ] Standard library
- [ ] Diagnostics
- [ ] Documentation (`docs/SPEC.md`)
- [ ] Tests (`tests/`)

## Alternatives considered
Other designs you weighed and why you rejected them.

## Notes
Kinetra `v0.0.1` is feature-frozen for patch releases; new language features
target the next minor version (see `docs/SPEC.md` §13 and the README roadmap).
Per `CONTRIBUTING.md`, discuss major designs here before opening a PR. 