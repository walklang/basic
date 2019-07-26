# Basic CMake Build Instructions

Basic comes with a CMake build script ([CMakeLists.txt](../CMakeLists.txt))
that can be used on a wide range of platforms ("C" stands for cross-platform.).
If you don't have CMake installed already, you can download it for free from
<https://www.cmake.org/>.

CMake works by generating native makefiles or build projects that can
be used in the compiler environment of your choice.

For API/ABI compatibility reasons, we strongly recommend building Basic in a
subdirectory of your project or as an embedded dependency.

## Incorporating Basic Into a CMake Project

The recommendations below are similar to those for using CMake within the
googletest framework
(<https://github.com/google/googletest/blob/master/googletest/README.md#incorporating-into-an-existing-cmake-project>)

### Step-by-Step Instructions

1. If you want to build the Basic tests, integrate the Basic dependency
[Google Test](https://github.com/google/googletest) into your CMake project. To disable Basic tests, you have to pass
`-DBUILD_TESTING=OFF` when configuring your project with CMake.

2. Download Basic and copy it into a subdirectory in your CMake project or add
Basic as a [git submodule](https://git-scm.com/docs/git-submodule) in your
CMake project.

3. You can then use the CMake command
[`add_subdirectory()`](https://cmake.org/cmake/help/latest/command/add_subdirectory.html)
to include Basic directly in your CMake project.

4. Add the **basic::** target you wish to use to the
[`target_link_libraries()`](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)
section of your executable or of your library.<br>
Here is a short CMakeLists.txt example of a project file using Basic.

```cmake
cmake_minimum_required(VERSION 3.5)
project(my_project)

# Pick the C++ standard to compile with.
# Basic currently supports C++11, C++14, and C++17.
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(abseil-cpp)

add_executable(my_exe source.cpp)
target_link_libraries(my_exe basic::base basic::synchronization basic::strings)
```

### Running Basic Tests with CMake

Use the `-DABSL_RUN_TESTS=ON` flag to run Basic tests.  Note that if the `-DBUILD_TESTING=OFF` flag is passed then Basic tests will not be run.

You will need to provide Basic with a Googletest dependency.  There are two
options for how to do this:

* Use `-DABSL_USE_GOOGLETEST_HEAD`.  This will automatically download the latest
Googletest source into the build directory at configure time.  Googletest will
then be compiled directly alongside Basic's tests.
* Manually integrate Googletest with your build.  See
https://github.com/google/googletest/blob/master/googletest/README.md#using-cmake
for more information on using Googletest in a CMake project.

For example, to run just the Basic tests, you could use this script:

```
cd path/to/abseil-cpp
mkdir build
cd build
cmake -DABSL_USE_GOOGLETEST_HEAD=ON -DABSL_RUN_TESTS=ON ..
make -j
ctest
```

Currently, we only run our tests with CMake in a Linux environment, but we are
working on the rest of our supported platforms. See
https://github.com/abseil/abseil-cpp/projects/1 and
https://github.com/abseil/abseil-cpp/issues/109 for more information.

### Available Basic CMake Public Targets

Here's a non-exhaustive list of Basic CMake public targets:

```cmake
basic::base
basic::algorithm
basic::debugging
basic::flat_hash_map
basic::memory
basic::meta
basic::numeric
basic::strings
basic::synchronization
basic::time
basic::utility
```
