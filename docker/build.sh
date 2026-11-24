#!/bin/sh
# Builds the mera400-dev image. The build context is this repository (the C
# library); the LLVM fork is a separate checkout, passed as the named build
# context "llvm-src".
#
#   docker/build.sh [path to mera400-llvm] [extra docker build arguments]
set -eu
here=$(cd "$(dirname "$0")/.." && pwd)
llvm=${1:-"$here/../mera400-llvm"}
[ $# -gt 0 ] && shift
exec docker build -f "$here/docker/Dockerfile" \
  --build-context "llvm-src=$llvm" -t mera400-dev "$@" "$here"
