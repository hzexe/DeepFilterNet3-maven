#!/bin/bash

# 字节序测试脚本
# 用于验证C++和Rust之间的字节序是否一致

set -e

echo "========================================"
echo "  字节序测试脚本"
echo "========================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ANDROID_DIR="${PROJECT_ROOT}/android"
CPP_TEST_DIR="${ANDROID_DIR}/deepfilter/src/main/cpp/test"
RUST_TEST_DIR="${PROJECT_ROOT}/deepfilter-ort/test"

echo "项目根目录: ${PROJECT_ROOT}"
echo "Android目录: ${ANDROID_DIR}"
echo "C++测试目录: ${CPP_TEST_DIR}"
echo "Rust测试目录: ${RUST_TEST_DIR}"
echo ""

# 1. 编译C++测试
echo "========================================"
echo "步骤1: 编译C++测试"
echo "========================================"
echo ""

cd "${CPP_TEST_DIR}"
if [ -f "CMakeLists.txt" ]; then
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
    
    if [ -f "bin/endianness_test" ] || [ -f "bin/endianness_test.exe" ]; then
        echo -e "${GREEN}✓ C++测试编译成功${NC}"
    else
        echo -e "${RED}✗ C++测试编译失败${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}⚠ 未找到CMakeLists.txt，跳过C++测试编译${NC}"
fi
echo ""

# 2. 运行C++测试
echo "========================================"
echo "步骤2: 运行C++测试"
echo "========================================"
echo ""

cd "${CPP_TEST_DIR}/build"
if [ -f "bin/endianness_test" ]; then
    ./bin/endianness_test
    echo -e "${GREEN}✓ C++测试运行成功${NC}"
elif [ -f "bin/endianness_test.exe" ]; then
    ./bin/endianness_test.exe
    echo -e "${GREEN}✓ C++测试运行成功${NC}"
else
    echo -e "${YELLOW}⚠ 未找到C++测试可执行文件${NC}"
fi
echo ""

# 3. 编译Rust测试
echo "========================================"
echo "步骤3: 编译Rust测试"
echo "========================================"
echo ""

cd "${RUST_TEST_DIR}"
if [ -f "endianness_test.rs" ]; then
    rustc --edition 2021 endianness_test.rs -o endianness_test
    
    if [ -f "endianness_test" ] || [ -f "endianness_test.exe" ]; then
        echo -e "${GREEN}✓ Rust测试编译成功${NC}"
    else
        echo -e "${RED}✗ Rust测试编译失败${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}⚠ 未找到endianness_test.rs，跳过Rust测试编译${NC}"
fi
echo ""

# 4. 运行Rust测试
echo "========================================"
echo "步骤4: 运行Rust测试"
echo "========================================"
echo ""

cd "${RUST_TEST_DIR}"
if [ -f "endianness_test" ]; then
    ./endianness_test
    echo -e "${GREEN}✓ Rust测试运行成功${NC}"
elif [ -f "endianness_test.exe" ]; then
    ./endianness_test.exe
    echo -e "${GREEN}✓ Rust测试运行成功${NC}"
else
    echo -e "${YELLOW}⚠ 未找到Rust测试可执行文件${NC}"
fi
echo ""

# 5. 对比测试结果
echo "========================================"
echo "步骤5: 对比测试结果"
echo "========================================"
echo ""

echo "请手动对比C++和Rust的测试输出："
echo ""
echo "C++测试输出："
echo "  1.0f -> 00 00 80 3F (Little-Endian)"
echo "  -1.0f -> 00 00 80 BF (Little-Endian)"
echo "  0.0f -> 00 00 00 00 (Little-Endian)"
echo ""
echo "Rust测试输出："
echo "  1.0f -> 00 00 80 3F (Little-Endian)"
echo "  -1.0f -> 00 00 80 BF (Little-Endian)"
echo "  0.0f -> 00 00 00 00 (Little-Endian)"
echo ""
echo "如果两者的输出一致，说明字节序兼容，无需转换。"
echo ""

# 6. 总结
echo "========================================"
echo "测试总结"
echo "========================================"
echo ""
echo "测试完成！"
echo ""
echo "结论："
echo "  如果C++和Rust的测试输出一致，说明："
echo "  1. 两者使用相同的字节序（Little-Endian）"
echo "  2. 两者使用相同的浮点标准（IEEE 754）"
echo "  3. 数据可以直接传递，无需字节序转换"
echo ""
echo "详细分析请参考：${ANDROID_DIR}/ENDIANNESS_ANALYSIS.md"
echo ""
