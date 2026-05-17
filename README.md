# libunwind

**libunwind** is a portable and efficient C API for determining the current call
chain of ELF program threads of execution and for resuming execution at any point
in that call chain. The API supports both local (same process) and remote (other
process) operation.

The API is useful in a number of applications, including but not limited to the
following.
- **program introspection**
    Either for error messages showing a back trace of the call chain leading to
    a problem or for performance monitoring and analysis.
- **debugging**
    Whether the debugging and analysis of the call chain of a remote program or
    the post-mortem analysis of a coredump.
- **language runtime exception handling**
    **libunwind** optionally provides an alternative implementation of the
    Itanium exception handling ABI used by many popular toolchains.
- **alternative `setjmp()`/`longjmp()`**
    **libunwind** optionally provides an alternative implementation of the
    `setjmp()`/`longjmp()` functionality of the C standard library.

## Supported Systems

[![CI - Linux (GNU)](https://github.com/libunwind/libunwind/actions/workflows/CI-linux-gnu.yml/badge.svg)](https://github.com/libunwind/libunwind/actions/workflows/CI-linux-gnu.yml)
[![CI - Linux (musl)](https://github.com/libunwind/libunwind/actions/workflows/CI-linux-musl.yml/badge.svg)](https://github.com/libunwind/libunwind/actions/workflows/CI-linux-musl.yml)
[![CI - FreeBSD](https://github.com/libunwind/libunwind/actions/workflows/CI-freebsd.yml/badge.svg)](https://github.com/libunwind/libunwind/actions/workflows/CI-freebsd.yml)
[![CI - Windows](https://github.com/libunwind/libunwind/actions/workflows/CI-win.yml/badge.svg)](https://github.com/libunwind/libunwind/actions/workflows/CI-win.yml)

This library supports several architecture/operating-system combinations:

| System  | Architecture | Status |
| :------ | :----------- | :----- |
| Linux   | x86-64       | ✓      |
| Linux   | x86          | ✓      |
| Linux   | ARM          | ✓      |
| Linux   | AArch64      | ✓      |
| Linux   | Alpha        | ✓      |
| Linux   | PPC32        | ✓      |
| Linux   | PPC64        | ✓      |
| Linux   | PPC64LE      | ✓      |
| Linux   | s390x        | ✓      |
| Linux   | SuperH       | ✓      |
| Linux   | IA-64        | ✓      |
| Linux   | PA-RISC      | Works well, but C library missing unwind-info |
| Linux   | MIPS         | ✓      |
| Linux   | RISC-V       | ✓      |
| Linux   | LoongArch    | 64-bit only |
| HP-UX   | IA-64        | Mostly works, but known to have serious limitations |
| FreeBSD | x86-64       | ✓      |
| FreeBSD | x86          | ✓      |
| FreeBSD | AArch64      | ✓      |
| FreeBSD | PPC32        | ✓      |
| FreeBSD | PPC64        | ✓      |
| FreeBSD | RISC-V       | ✓      |
| QNX     | Aarch64      | ✓      |
| QNX     | x86-64       | ✓      |
| Solaris | x86-64       | ✓      |

## Libc Requirements

libunwind depends on getcontext(), setcontext() functions which are missing
from C libraries like musl-libc because they are considered to be "obsolescent"
API by POSIX document.  The following table tries to track current status of
such dependencies

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
|    x86\_64   |     p      |      p     |

## General Build Instructions

In general, this library can be built and installed with the following
commands:

    $ autoreconf -i # Needed only for building from git. Depends on libtool.
    $ ./configure --prefix=PREFIX
    $ make
    $ make install

where `PREFIX` is the installation prefix.  By default, a prefix of
`/usr/local` is used, such that `libunwind.a` is installed in
`/usr/local/lib` and `unwind.h` is installed in `/usr/local/include`.

## Regression Testing

After building the library, you can run a set of regression tests with:

    $ make check

## Performance Testing

This distribution includes a few simple performance tests which give
some idea of the basic cost of various libunwind operations.  After
building the library, you can run these tests with the following
commands:

    $ cd tests
    $ make perf

## Contacting the Developers

Please raise issues and pull requests through the GitHub repository:
<https://github.com/libunwind/libunwind>.
