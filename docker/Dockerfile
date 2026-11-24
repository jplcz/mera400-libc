# syntax=docker/dockerfile:1.4
#
# MERA-400 C and C++ development image. Build it with docker/build.sh (it
# passes the LLVM source as the named build context "llvm-src"); see
# docker/README.md.

ARG BASE=ubuntu:24.04
ARG PREFIX=/opt/mera400

# ---------------------------------------------------------------------------
# Stage 1 (toolchain): build the LLVM fork (clang, lld, lldb and the binary utilities).
# ---------------------------------------------------------------------------
FROM ${BASE} AS toolchain
ARG PREFIX
# Assertions make the compiler slower but catch compiler bugs; off by default.
ARG LLVM_ASSERTIONS=OFF
RUN apt-get update && apt-get install -y --no-install-recommends \
      ca-certificates cmake ninja-build g++ python3 && \
    rm -rf /var/lib/apt/lists/*
# X86 is the host target the tools run on; Mera400 is the experimental target.
# The optional dependencies (zlib, zstd, XML, terminfo, Python, Lua, libedit)
# are switched off so the final image needs no shared libraries but libc/libstdc++.
# The LLVM source is bind-mounted from the named context, not copied.
RUN --mount=type=bind,from=llvm-src,target=/src \
    cmake -S /src/llvm -B /build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=${PREFIX} \
      -DLLVM_ENABLE_PROJECTS="clang;lld;lldb" \
      -DLLVM_TARGETS_TO_BUILD=X86 \
      -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=Mera400 \
      -DLLVM_ENABLE_ASSERTIONS=${LLVM_ASSERTIONS} \
      -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_ZSTD=OFF \
      -DLLVM_ENABLE_LIBXML2=OFF -DLLVM_ENABLE_TERMINFO=OFF \
      -DLLDB_ENABLE_PYTHON=OFF -DLLDB_ENABLE_LUA=OFF \
      -DLLDB_ENABLE_LIBEDIT=OFF -DLLDB_ENABLE_CURSES=OFF \
      -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
      -DLLVM_INCLUDE_BENCHMARKS=OFF -DCLANG_INCLUDE_TESTS=OFF \
      -DLLDB_INCLUDE_TESTS=OFF \
      -DLLVM_INSTALL_TOOLCHAIN_ONLY=OFF \
      -DLLVM_DISTRIBUTION_COMPONENTS="clang;lld;lldb;lldb-server;llvm-mc;llvm-ar;llvm-ranlib;llvm-nm;llvm-objdump;llvm-objcopy;llvm-readelf;llvm-readobj;llvm-size;llvm-strip;clang-resource-headers" && \
    ninja -C /build install-distribution

# ---------------------------------------------------------------------------
# Stage 2 (clibrary): the C library, for both memory models, installed into the sysroot.
# The driver's default sysroot is <bindir>/../mera400, hence that name.
# ---------------------------------------------------------------------------
FROM ${BASE} AS clibrary
ARG PREFIX
RUN apt-get update && apt-get install -y --no-install-recommends \
      cmake ninja-build make && \
    rm -rf /var/lib/apt/lists/*
COPY --from=toolchain ${PREFIX} ${PREFIX}
COPY . /libc
RUN for model in small large; do \
      cmake -S /libc -B /build-$model -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=/libc/cmake/mera400-toolchain.cmake \
        -DLLVM_BIN_DIR=${PREFIX}/bin \
        -DMERA400_MODEL=$model -DBUILD_TESTING=OFF && \
      ninja -C /build-$model && \
      cmake --install /build-$model --prefix ${PREFIX}/mera400 || exit 1; \
    done

# ---------------------------------------------------------------------------
# Stage 3: the libc++ headers (header-only: no compiled libc++, so nothing to
# link; the C++ runtime pieces live in the libc).
# ---------------------------------------------------------------------------
FROM ${BASE} AS libcxx
ARG PREFIX
RUN apt-get update && apt-get install -y --no-install-recommends \
      cmake ninja-build python3 && \
    rm -rf /var/lib/apt/lists/*
COPY --from=toolchain ${PREFIX} ${PREFIX}
COPY --from=clibrary ${PREFIX}/mera400 ${PREFIX}/mera400
RUN --mount=type=bind,from=llvm-src,target=/src \
    cmake -S /src/runtimes -B /build -G Ninja \
      -DCMAKE_C_COMPILER=${PREFIX}/bin/clang \
      -DCMAKE_CXX_COMPILER=${PREFIX}/bin/clang++ \
      -DCMAKE_C_COMPILER_TARGET=mera400 -DCMAKE_CXX_COMPILER_TARGET=mera400 \
      -DCMAKE_SYSTEM_NAME=Generic \
      -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
      -DCMAKE_INSTALL_PREFIX=${PREFIX}/mera400 \
      -DLLVM_ENABLE_RUNTIMES=libcxx \
      -DLIBCXX_CXX_ABI=none -DLIBCXX_ENABLE_SHARED=OFF \
      -DLIBCXX_ENABLE_EXCEPTIONS=OFF -DLIBCXX_ENABLE_RTTI=OFF \
      -DLIBCXX_ENABLE_THREADS=OFF -DLIBCXX_ENABLE_FILESYSTEM=OFF \
      -DLIBCXX_ENABLE_LOCALIZATION=OFF -DLIBCXX_ENABLE_WIDE_CHARACTERS=OFF \
      -DLIBCXX_ENABLE_RANDOM_DEVICE=OFF -DLIBCXX_ENABLE_MONOTONIC_CLOCK=OFF \
      -DLIBCXX_ENABLE_UNICODE=OFF -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF \
      -DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF && \
    ninja -C /build install-cxx-headers

# ---------------------------------------------------------------------------
# Final image: only the toolchain and the sysroot.
# ---------------------------------------------------------------------------
FROM ${BASE}
ARG PREFIX
RUN apt-get update && apt-get install -y --no-install-recommends \
      libstdc++6 && \
    rm -rf /var/lib/apt/lists/*
COPY --from=libcxx ${PREFIX} ${PREFIX}
ENV PATH=${PREFIX}/bin:${PATH}
WORKDIR /work
