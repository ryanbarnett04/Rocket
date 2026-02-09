cmake_minimum_required(VERSION 3.20)

project(RocketClang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM and Clang via vcpkg
find_package(LLVM CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_executable(RocketClang
    Rocket-Clang/Rocket-Clang.cpp
)

target_include_directories(RocketClang PRIVATE
    ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(RocketClang PRIVATE
    clangTooling
    clangBasic
    clangAST
    clangFrontend
    clangLex
    clangAnalysis
)