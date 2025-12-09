cmake_minimum_required(VERSION 3.15)
project(filerecover_engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(filerecover_engine STATIC
    src/fr_stub.cpp
)

# 将 disk_io_stub.cpp 纳入库实现中
target_sources(filerecover_engine PRIVATE src/disk_io_stub.cpp)

target_include_directories(filerecover_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(BUILD_ENGINE_TESTS "Build engine unit tests" ON)
if(BUILD_ENGINE_TESTS)
  enable_testing()
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(engine_tests
    ../tests/engine_test.cpp
  )
  target_link_libraries(engine_tests PRIVATE filerecover_engine gtest_main)
  add_test(NAME EngineTests COMMAND engine_tests)

  # 新增 disk_io 单元测试
  add_executable(disk_io_tests
    ../tests/disk_io_test.cpp
  )
  target_link_libraries(disk_io_tests PRIVATE filerecover_engine gtest_main)
  add_test(NAME DiskIOTests COMMAND disk_io_tests)
endif()
