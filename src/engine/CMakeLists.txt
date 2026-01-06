cmake_minimum_required(VERSION 3.15)
project(filerecover_engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(filerecover_engine STATIC
    src/fr_stub.cpp
)

# 将 disk_io_stub.cpp 纳入库实现中
if(WIN32)
  # Use Windows native DiskIO implementation when building on Windows
  target_sources(filerecover_engine PRIVATE src/disk_io_win.cpp)
else()
  target_sources(filerecover_engine PRIVATE src/disk_io_stub.cpp)
endif()
target_sources(filerecover_engine PRIVATE src/ntfs_mft.cpp)
target_sources(filerecover_engine PRIVATE src/ntfs_stub.cpp)
target_sources(filerecover_engine PRIVATE src/ntfs_capi.cpp)

target_include_directories(filerecover_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# MinGW runtime handling: optionally link libstdc++/libgcc statically so
# exercises and small tools don't require shipping DLLs alongside binaries.
option(USE_STATIC_MINGW_RUNTIME "Statically link libstdc++ and libgcc when using MinGW" OFF)
if(MINGW AND USE_STATIC_MINGW_RUNTIME)
  message(STATUS "MinGW detected: enabling -static-libgcc -static-libstdc++ for filerecover_engine")
  # target_link_options requires CMake >= 3.13; project uses 3.15 so it's available.
  target_link_options(filerecover_engine PRIVATE "-static-libgcc" "-static-libstdc++")
endif()

option(BUILD_ENGINE_TESTS "Build engine unit tests" ON)
if(BUILD_ENGINE_TESTS)
  enable_testing()

  # Prefer a vendored googletest under third_party/googletest if present.
  # This avoids network downloads during configure and makes CI/offline builds stable.
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest googletest-build)
  else()
    message(WARNING "googletest not found under third_party/googletest; falling back to FetchContent which may require network access")
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )
    FetchContent_MakeAvailable(googletest)
  endif()

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

  # NTFS parser tests
  add_executable(ntfs_tests
    ../tests/ntfs_test.cpp
  )
  target_link_libraries(ntfs_tests PRIVATE filerecover_engine gtest_main)
  add_test(NAME NTFSParserTests COMMAND ntfs_tests)
endif()
