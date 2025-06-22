#!/usr/bin/env bash

type="${1:-"Debug"}"

here="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

run() {
  local prefix="${1?no prefix}"
  shift 1
  cmake_options=(
    '-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
    "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=$prefix/cmake-conan/conan_provider.cmake"
    "-DCMAKE_BUILD_TYPE=$type"
  )
  local cross_build="/cross-build"
  local build_dir="$prefix$cross_build"
  mkdir -p "$here$cross_build"
  podman run --rm -it --userns=keep-id -v "$here:$prefix:ro" -v "$here$cross_build:$build_dir" --workdir "$build_dir" -e CONAN_HOME="$build_dir/conan-home" "docker.io/dockcross/windows-static-x64:20250109-7bf589c" "$@"
}

run /work conan profile detect -f
run /work cmake .. "${cmake_options[@]}"
run /work make -j10