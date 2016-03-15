---
version: 8.0.0-dp-9
---
# Make "the effective rect of stack" more accurate on Linux

Because the engine began using GDK on Linux in LiveCode 7.0, there is
now a better method for computing the **effective rect** of a
window. The engine has been updated to use this method, rather than
the heuristic which was there before.
