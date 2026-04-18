cmake_minimum_required(VERSION 3.14)
project(CRG_Benchmark)

# On force le C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Génère le fichier pour réparer l'IntelliSense de VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# On télécharge Google Benchmark
include(FetchContent)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# On indique où se trouve le fichier cpp (dans le dossier demo)
add_executable(crg_benchmark benchmarks/crg_benchmark.cpp)

# On dit au compilateur de chercher les headers (.hpp) dans le dossier demo
target_include_directories(crg_benchmark PRIVATE benchmarks)

# On lie Google Benchmark et le multithreading
target_link_libraries(crg_benchmark PRIVATE benchmark::benchmark pthread)