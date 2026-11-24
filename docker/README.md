# Docker image for MERA-400 C and C++ development

## Why

Getting a working toolchain by hand means building the LLVM fork (clang, the
compiler; lld, the linker; lldb, the debugger), building this C library with
it, installing the result into a *sysroot* (a directory with `include/` and
`lib/` that the compiler treats as the target's `/usr`), and knowing the right
flags. The image does all of that once, so the only thing you need on the
host is Docker (a tool that runs programs in packaged, reproducible
environments called containers).

## Build

The LLVM fork is a separate checkout (it is not part of this repository), so
it is passed as the named build context `llvm-src`:

```sh
docker/build.sh                      # expects ../mera400-llvm
docker/build.sh /path/to/mera400-llvm
```

The image is only built locally (nothing is published) and is tagged
`mera400-dev`. The first build compiles LLVM and takes a long time (about an
hour or more, depending on the cores); Docker caches the stages, so later
builds that change only the libc are fast. Pass `--build-arg
LLVM_ASSERTIONS=ON` through `build.sh` for a compiler with internal checks.

## What is inside

```
/opt/mera400/bin       clang, clang++, ld.lld, lldb, llvm-mc/-objdump/-ar/-nm/...
/opt/mera400/mera400   the sysroot: include/, include/c++/v1/ (libc++ headers),
                       lib/ (small memory model), lib/large/ (large model)
```

The sysroot is next to `bin/` because that is where the clang driver looks by
default, so no `--sysroot` flag is needed. C++ is header-only: libc++'s
headers are installed, and the C++ runtime pieces (`operator new`,
`__cxa_guard_*`, ...) are in the libc. Exceptions and RTTI (run-time type
information) are off for the target.

## Use

```sh
docker run --rm -v "$PWD":/work mera400-dev \
  clang --target=mera400-unknown-none -O2 examples/hello.c -o hello.elf
docker run --rm -v "$PWD":/work mera400-dev \
  clang++ --target=mera400-unknown-none -O2 examples/shapes.cpp -o shapes.elf
```

Add `-mlarge-memory` for the large memory model (32-bit pointers). The em400
emulator is not part of the image; use `llvm-objcopy -O binary` (the build
already does this for the examples) to get a flat image for it.
