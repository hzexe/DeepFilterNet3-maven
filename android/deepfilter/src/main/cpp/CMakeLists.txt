cmake_minimum_required(VERSION 3.18.1)

project("deepfilter_native")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Android日志库
find_library(log-lib log)

# 查找AAudio库（Android 8.1+ API 26+）
find_library(aaudio-lib aaudio)

# 设置头文件包含路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 添加音频处理器源文件（已集成录制和降噪功能）
add_library(
    deepfilter_native
    SHARED
    src/AudioProcessor.cpp
    src/jni_interface.cpp
)

# 链接系统库
# 注意：AAudio是Android 8.1+ (API 26+)引入的系统库
# 需要链接aaudio库和android库来使用AAudio API
target_link_libraries(
    deepfilter_native
    ${log-lib}
    ${aaudio-lib}
    android
)

# 链接deepfilter-ort动态库
# deepfilter-ort.so由Rust编译生成，位于jniLibs目录
# 由于C++代码直接调用了Rust库的函数，需要在链接时链接该库
add_library(deepfilter_ort SHARED IMPORTED)
set_target_properties(deepfilter_ort PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}/libdeepfilter_ort.so"
)
target_link_libraries(deepfilter_native deepfilter_ort)