cmake_minimum_required(VERSION 3.16)
project(chess_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(chesscore STATIC
  src/bitboard.cpp
  src/position.cpp
  src/movegen.cpp
  src/perft.cpp
  src/eval.cpp
  src/search.cpp
)
target_include_directories(chesscore PUBLIC include)

add_executable(chess src/main.cpp)
target_link_libraries(chess PRIVATE chesscore)

add_executable(chess_tests tests/test_main.cpp)
target_link_libraries(chess_tests PRIVATE chesscore)
