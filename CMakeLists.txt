#
#
# Copyright (c) 2017 The Authors of ANT(http:://ant.sh) . All Rights Reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#


# Most widely used distributions have cmake 3.5 or greater available as of March 2019.
cmake_minimum_required(VERSION 3.5)

# Compiler id for Apple Clang is now AppleClang.
cmake_policy(SET CMP0025 NEW)

# if command can use IN_LIST
cmake_policy(SET CMP0057 NEW)

# Project version variables are the empty std::string if version is unspecified
cmake_policy(SET CMP0048 NEW)

project(basic CXX)

# when basic is included as subproject
# in the source tree of a project that uses it, install rules are disabled.
if(NOT "^${CMAKE_SOURCE_DIR}$" STREQUAL "^${PROJECT_SOURCE_DIR}$")
  set(ABSL_ENABLE_INSTALL FALSE)
else()
  set(ABSL_ENABLE_INSTALL TRUE)
endif()

list(APPEND CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_LIST_DIR}/CMake
  ${CMAKE_CURRENT_LIST_DIR}/basic/copts
)

include(BasicInstallDirs)
include(CMakePackageConfigHelpers)
include(BasicHelpers)


# include current path
list(APPEND BASIC_COMMON_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(BASIC_USING_CLANG ON)
else()
  set(BASIC_USING_CLANG OFF)
endif()

# find dependencies
## pthread
find_package(Threads REQUIRED)

option(BASIC_USE_GOOGLETEST_HEAD
  "If ON, basic will download HEAD from googletest at config time." OFF)

option(BASIC_RUN_TESTS "If ON, Basic tests will be run." OFF)

if(${BASIC_RUN_TESTS})
  # enable CTest.  This will set BUILD_TESTING to ON unless otherwise specified
  # on the command line
  include(CTest)
  enable_testing()
endif()

## check targets
if(BUILD_TESTING)

  if(${BASIC_USE_GOOGLETEST_HEAD})
    include(CMake/Googletest/DownloadGTest.cmake)
    set(basic_gtest_src_dir ${CMAKE_BINARY_DIR}/googletest-src)
    set(basic_gtest_build_dir ${CMAKE_BINARY_DIR}/googletest-build)
  endif()

  check_target(gtest)
  check_target(gtest_main)
  check_target(gmock)

  list(APPEND BASIC_TEST_COMMON_LIBRARIES
    gtest_main
    gtest
    gmock
    ${CMAKE_THREAD_LIBS_INIT}
  )
endif()

add_subdirectory(basic)

if(BASIC_ENABLE_INSTALL)
  # basic:lts-remove-begin(system installation is supported for LTS releases)
  # We don't support system-wide installation
  list(APPEND SYSTEM_INSTALL_DIRS "/usr/local" "/usr" "/opt/" "/opt/local" "c:/Program Files/${PROJECT_NAME}")
  if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX IN_LIST SYSTEM_INSTALL_DIRS)
    message(WARNING "\
  The default and system-level install directories are unsupported except in LTS \
  releases of Basic.  Please set CMAKE_INSTALL_PREFIX to install Basic in your \
  source or build tree directly.\
    ")
  endif()
  # basic:lts-remove-end

  # install as a subdirectory only
  install(EXPORT ${PROJECT_NAME}Targets
    NAMESPACE basic::
    DESTINATION "${BASIC_INSTALL_CONFIGDIR}"
  )

  configure_package_config_file(
    CMake/basicConfig.cmake.in
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${BASIC_INSTALL_CONFIGDIR}"
  )
  install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    DESTINATION "${BASIC_INSTALL_CONFIGDIR}"
  )

  if(basic_VERSION)
    write_basic_package_version_file(
      "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
      COMPATIBILITY ExactVersion
    )

    install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
      DESTINATION ${BASIC_INSTALL_CONFIGDIR}
    )
  endif()  # basic_VERSION

  install(DIRECTORY basic
    DESTINATION ${BASIC_INSTALL_INCLUDEDIR}
    FILES_MATCHING
      PATTERN "*.inc"
      PATTERN "*.h"
  )
endif()  # BASIC_ENABLE_INSTALL
