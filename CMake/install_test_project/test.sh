#!/bin/bash
#
# Copyright 2019 The Basic Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# "Unit" and integration tests for Absl CMake installation

# TODO(basic-team): This script isn't fully hermetic because
# -DABSL_USE_GOOGLETEST_HEAD=ON means that this script isn't pinned to a fixed
# version of GoogleTest. This means that an upstream change to GoogleTest could
# break this test. Fix this by allowing this script to pin to a known-good
# version of GoogleTest.

# Fail on any error. Treat unset variables an error. Print commands as executed.
set -euox pipefail

install_basic() {
  pushd "${basic_build_dir}"
  if [[ "${#}" -eq 1 ]]; then
    cmake -DCMAKE_INSTALL_PREFIX="${1}" "${basic_dir}"
  else
    cmake "${basic_dir}"
  fi
  cmake --build . --target install -- -j
  popd
}

uninstall_basic() {
  xargs rm < "${basic_build_dir}"/install_manifest.txt
  rm -rf "${basic_build_dir}"
  mkdir -p "${basic_build_dir}"
}

lts_install=""

while getopts ":l" lts; do
  case "${lts}" in
    l )
      lts_install="true"
      ;;
  esac
done

basic_dir=/abseil-cpp
basic_build_dir=/buildfs/basic-build
project_dir="${basic_dir}"/CMake/install_test_project
project_build_dir=/buildfs/project-build

mkdir -p "${basic_build_dir}"
mkdir -p "${project_build_dir}"

if [[ "${lts_install}" ]]; then
  install_dir="/usr/local"
else
  install_dir="${project_build_dir}"/install
fi
mkdir -p "${install_dir}"

# Test build, install, and link against installed abseil
pushd "${project_build_dir}"
if [[ "${lts_install}" ]]; then
  install_basic
  cmake "${project_dir}"
else
  install_basic "${install_dir}"
  cmake "${project_dir}" -DCMAKE_PREFIX_PATH="${install_dir}"
fi

cmake --build . --target simple

output="$(${project_build_dir}/simple "printme" 2>&1)"
if [[ "${output}" != *"Arg 1: printme"* ]]; then
  echo "Faulty output on simple project:"
  echo "${output}"
  exit 1
fi

popd

# Test that we haven't accidentally made basic::basicblah
pushd "${install_dir}"

# Starting in CMake 3.12 the default install dir is lib$bit_width
if [[ -d lib64 ]]; then
  libdir="lib64"
elif [[ -d lib ]]; then
  libdir="lib"
else
  echo "ls *, */*, */*/*:"
  ls *
  ls */*
  ls */*/*
  echo "unknown lib dir"
fi

if [[ "${lts_install}" ]]; then
  # LTS versions append the date of the release to the subdir.
  # 9999/99/99 is the dummy date used in the local_lts workflow.
  basic_subdir="basic_99999999"
else
  basic_subdir="basic"
fi

if ! grep basic::strings "${libdir}/cmake/${basic_subdir}/basicTargets.cmake"; then
  cat "${libdir}"/cmake/basic/basicTargets.cmake
  echo "CMake targets named incorrectly"
  exit 1
fi

uninstall_basic
popd

if [[ ! "${lts_install}" ]]; then
  # Test that we warn if installed without a prefix or a system prefix
  output="$(install_basic 2>&1)"
  if [[ "${output}" != *"Please set CMAKE_INSTALL_PREFIX"* ]]; then
    echo "Install without prefix didn't warn as expected. Output:"
    echo "${output}"
    exit 1
  fi
  uninstall_basic

  output="$(install_basic /usr 2>&1)"
  if [[ "${output}" != *"Please set CMAKE_INSTALL_PREFIX"* ]]; then
    echo "Install with /usr didn't warn as expected. Output:"
    echo "${output}"
    exit 1
  fi
  uninstall_basic
fi

echo "Install test complete!"
exit 0
