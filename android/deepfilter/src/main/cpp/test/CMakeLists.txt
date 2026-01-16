cmake_minimum_required(VERSION 3.18.1)
project(EndiannessTest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(endianness_test
    test/endianness_test.cpp
)

# 设置输出目录
set_target_properties(endianness_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 打印编译信息
message(STATUS "Endianness Test Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output Directory: ${CMAKE_BINARY_DIR}/bin")
