# cmake

* `mera400-toolchain.cmake` - the *toolchain file* that tells CMake this is
  a cross build for a processor it does not know. It finds `llvm-mc`,
  `ld.lld`, `llvm-ar`, `llvm-objdump` in `LLVM_BIN_DIR` (if set) or `PATH`.
  Pass it with `-DCMAKE_TOOLCHAIN_FILE=`.
* `Mera400.cmake` - functions `mera400_add_library()` (assemble sources and
  archive them) and `mera400_add_executable()` (link with crt0 first and the
  standard linker script). CMake cannot assemble MERA-400 code itself, so
  these run the LLVM tools through custom commands.
