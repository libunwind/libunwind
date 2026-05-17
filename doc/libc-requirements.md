# Libc Requirements

libunwind depends on `getcontext()` and `setcontext()` functions which are
missing from C libraries like musl-libc because they are considered to be
"obsolescent" API by POSIX. The following table tracks the current status of
such dependencies.

 - r, requires
 - p, provides its own implementation
 - empty, no requirement

| Architecture | getcontext | setcontext |
|--------------|------------|------------|
|    aarch64   |     p      |      p     |
|    alpha     |     p      |      p     |
|    arm       |     p      |            |
|    hppa      |     p      |      p     |
|    ia64      |     p      |      r     |
|    loongarch |     p      |            |
|    mips      |     p      |            |
|    ppc32     |     r      |            |
|    ppc64     |     r      |      r     |
|    riscv     |     p      |      p     |
|    s390x     |     p      |      p     |
|    sh        |     r      |            |
|    x86       |     p      |      r     |
|    x86_64    |     p      |      p     |
