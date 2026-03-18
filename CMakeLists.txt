cmake_minimum_required(VERSION 3.24)
project(big-o-demo-hash-tables LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} src/main.cpp)

# Pass repo root so the exe can find graph.py and write results.csv there
target_compile_definitions(${PROJECT_NAME} PRIVATE
    REPO_DIR="${CMAKE_SOURCE_DIR}"
)
