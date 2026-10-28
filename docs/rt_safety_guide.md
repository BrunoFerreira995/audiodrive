# Real-time Safety Guide

## Rules
- Never allocate memory in audio callback
- Never use locks/mutexes in render path
- Never do file I/O or syscalls that block
- Never throw exceptions from real-time callback
- Use lock-free data structures for cross-thread comms
- Pre-allocate all buffers

## Audit
Run `python3 scripts/rt_audit.py` to check render paths.
