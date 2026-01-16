#include <iostream>
#include <iomanip>
#include <cstdint>

/**
 * 字节序测试工具
 * 
 * 用于验证C++和Rust之间的字节序是否一致
 */

/**
 * 打印浮点数的字节表示
 * 
 * @param value 浮点数值
 * @param name 名称
 */
void printFloatBytes(float value, const char* name) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    
    std::cout << name << " = " << std::fixed << std::setprecision(6) << value << std::endl;
    std::cout << "  Bytes: ";
    for (int i = 0; i < 4; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(bytes[i]) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // 判断字节序
    if (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x80 && bytes[3] == 0x3F) {
        std::cout << "  字节序: Little-Endian (小端序)" << std::endl;
    } else if (bytes[0] == 0x3F && bytes[1] == 0x80 && bytes[2] == 0x00 && bytes[3] == 0x00) {
        std::cout << "  字节序: Big-Endian (大端序)" << std::endl;
    } else {
        std::cout << "  字节序: 未知" << std::endl;
    }
    std::cout << std::endl;
}

/**
 * 测试IEEE 754浮点数表示
 */
void testIEEE754() {
    std::cout << "========================================" << std::endl;
    std::cout << "IEEE 754浮点数测试" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    // 测试1.0f
    printFloatBytes(1.0f, "1.0f");
    
    // 测试-1.0f
    printFloatBytes(-1.0f, "-1.0f");
    
    // 测试0.0f
    printFloatBytes(0.0f, "0.0f");
    
    // 测试3.1415926535f
    printFloatBytes(3.1415926535f, "3.1415926535f");
    
    // 测试123.456f
    printFloatBytes(123.456f, "123.456f");
}

/**
 * 测试音频数据字节序
 */
void testAudioData() {
    std::cout << "========================================" << std::endl;
    std::cout << "音频数据字节序测试" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    // 模拟音频数据（正弦波）
    const int numSamples = 10;
    float audioData[numSamples];
    
    std::cout << "生成正弦波音频数据：" << std::endl;
    for (int i = 0; i < numSamples; i++) {
        audioData[i] = std::sin(2.0 * 3.1415926535 * i / numSamples);
    }
    
    std::cout << "前5个采样点的字节表示：" << std::endl;
    for (int i = 0; i < 5; i++) {
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&audioData[i]);
        std::cout << "  Sample[" << i << "] = " << std::fixed << std::setprecision(6) 
                  << audioData[i] << " -> ";
        for (int j = 0; j < 4; j++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(bytes[j]) << " ";
        }
        std::cout << std::dec << std::endl;
    }
    std::cout << std::endl;
}

/**
 * 测试字节序一致性
 */
void testEndiannessConsistency() {
    std::cout << "========================================" << std::endl;
    std::cout << "字节序一致性测试" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    // 测试多个值，确保字节序一致
    float testValues[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    bool isLittleEndian = true;
    
    std::cout << "检查字节序一致性：" << std::endl;
    for (int i = 0; i < 5; i++) {
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&testValues[i]);
        
        // 检查是否为小端序
        // 小端序：最低有效字节在最低地址
        // 对于1.0f (0x3F800000)，小端序应该是 00 00 80 3F
        // 对于2.0f (0x40000000)，小端序应该是 00 00 00 40
        
        std::cout << "  " << testValues[i] << "f -> ";
        for (int j = 0; j < 4; j++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(bytes[j]) << " ";
        }
        std::cout << std::dec << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "结论：" << std::endl;
    std::cout << "  如果所有值的字节序都是 Little-Endian，" << std::endl;
    std::cout << "  则C++和Rust之间不需要字节序转换。" << std::endl;
    std::cout << std::endl;
}

/**
 * 主函数
 */
int main() {
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  字节序测试工具" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    testIEEE754();
    testAudioData();
    testEndiannessConsistency();
    
    std::cout << "========================================" << std::endl;
    std::cout << "  测试完成" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
