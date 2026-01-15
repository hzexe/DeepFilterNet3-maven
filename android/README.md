# DeepFilterNet Android Library

基于 DeepFilterNet3 的 Android 音频降噪库，提供高效的实时音频降噪功能。

## 项目简介

本项目将 DeepFilterNet3 音频降噪引擎封装为 Android AAR 库，支持 ARM64 架构的 Android 设备。使用 Rust 编写核心降噪算法，通过 JNI 接口提供 Java 调用，确保高性能和低延迟。

## 主要特性

- 🚀 高性能：基于 Rust 实现核心算法，确保高效的音频处理
- 🎯 实时降噪：支持实时音频流处理，延迟低
- 📦 易于集成：提供 AAR 包，方便集成到 Android 项目
- 🔧 灵活配置：支持自定义降噪参数
- 📱 ARM64 支持：针对 ARM64 架构优化
- 🎵 单声道支持：专注于单声道音频降噪

## 系统要求

- Android API 21+ (Android 5.0+)
- ARM64 架构设备
- Java 8+

## 项目结构

```
DeepFilterNet3-maven/
├── android/                    # Android 库项目
│   ├── deepfilter/            # 主库模块
│   │   ├── src/main/
│   │   │   ├── java/          # Java 源码
│   │   │   │   └── com/hzexe/audio/ns/
│   │   │   │       └── DeepFilterNet.java
│   │   │   ├── jniLibs/       # 原生库
│   │   │   │   └── arm64-v8a/
│   │   │   │       └── libdeepfilter_ort.so
│   │   │   └── AndroidManifest.xml
│   │   ├── build.gradle       # 库构建配置
│   │   └── proguard-rules.pro
│   ├── build.gradle           # 项目构建配置
│   ├── settings.gradle        # 项目设置
│   ├── gradlew              # Gradle wrapper (Unix)
│   ├── gradlew.bat          # Gradle wrapper (Windows)
│   └── gradle/             # Gradle wrapper 文件
├── deepfilter-ort/            # Rust 核心库
│   ├── src/
│   │   └── lib.rs            # Rust 源码
│   └── Cargo.toml            # Rust 配置
└── DeepFilterNet/             # DeepFilterNet 原始项目
    └── libDF/                # DeepFilterNet 核心库
```

## 快速开始

### 方式一：使用预编译的 AAR 包

1. 从 [Releases](../../releases) 页面下载最新版本的 `deepfilter-release.aar`
2. 将 AAR 文件复制到 Android 项目的 `app/libs` 目录
3. 在 `app/build.gradle` 中添加依赖：

```gradle
dependencies {
    implementation(fileTree(dir: "libs", include: ["*.aar"]))
}
```

### 方式二：从源码构建

#### 环境要求

- [Android Studio](https://developer.android.com/studio)（包含 Android SDK 和 NDK）
- [Rust](https://www.rust-lang.org/tools/install)
- Gradle（项目已包含 Gradle wrapper）

#### 构建方法

使用 Gradle 构建系统，会自动编译 Rust 项目并构建 AAR 包：

```bash
# 构建 Release 版本
cd android
gradlew.bat :deepfilter:assembleRelease

# 构建 Debug 版本
cd android
gradlew.bat :deepfilter:assembleDebug
```

**构建流程**：
1. 自动编译 Rust 项目（`deepfilter-ort`）
2. 复制 `.so` 文件到 `jniLibs/arm64-v8a/` 目录
3. 构建 Android AAR 包
4. AAR 文件自动包含 `.so` 文件

**输出位置**：
- Release AAR: `deepfilter/build/outputs/aar/deepfilter-release.aar`
- Debug AAR: `deepfilter/build/outputs/aar/deepfilter-debug.aar`

#### 手动编译 Rust 项目（可选）

如果只想编译 Rust 项目而不构建 AAR：

```bash
cd android
gradlew.bat compileNative
```

这将编译 `deepfilter-ort` 项目并复制 `.so` 文件到 `jniLibs` 目录。

## 使用方法

### 基本使用

```java
import com.hzexe.audio.ns.DeepFilterNet;

public class AudioProcessor {
    private DeepFilterNet deepFilterNet;
    
    // 初始化
    public void init(byte[] modelBytes) {
        // 创建 DeepFilterNet 实例
        deepFilterNet = new DeepFilterNet(modelBytes);
        
        // 初始化引擎
        // postFilterBeta: 后滤波器 beta 参数（控制降噪强度，>0 启用后滤波）
        // attenLimDb: 衰减限制（dB），控制最大降噪幅度
        boolean success = deepFilterNet.initialize(0.5f, 30.0f);
        
        if (!success) {
            throw new RuntimeException("DeepFilterNet 初始化失败");
        }
    }
    
    // 处理音频（float 数组）
    public void processAudio(float[] input, float[] output) {
        if (!deepFilterNet.isInitialized()) {
            throw new RuntimeException("DeepFilterNet 未初始化");
        }
        
        // 处理音频帧
        float lsnr = deepFilterNet.process(input, output);
        
        if (lsnr < 0) {
            // 处理失败
            Log.e("DeepFilterNet", "音频处理失败");
        }
    }
    
    // 处理音频（byte 数组，f32 格式）
    public void processAudio(byte[] input, byte[] output) {
        if (!deepFilterNet.isInitialized()) {
            throw new RuntimeException("DeepFilterNet 未初始化");
        }
        
        // 处理音频帧
        float lsnr = deepFilterNet.process(input, output);
        
        if (lsnr < 0) {
            // 处理失败
            Log.e("DeepFilterNet", "音频处理失败");
        }
    }
    
    // 释放资源
    public void release() {
        if (deepFilterNet != null) {
            deepFilterNet.release();
            deepFilterNet = null;
        }
    }
}
```

### 加载模型文件

DeepFilterNet 需要加载模型文件（tar.gz 格式）。以下是加载模型文件的示例：

```java
private byte[] loadModelFile(Context context, String assetPath) throws IOException {
    InputStream is = context.getAssets().open(assetPath);
    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
    
    int nRead;
    byte[] data = new byte[16384];
    
    while ((nRead = is.read(data, 0, data.length)) != -1) {
        buffer.write(data, 0, nRead);
    }
    
    buffer.flush();
    is.close();
    
    return buffer.toByteArray();
}

// 使用
byte[] modelBytes = loadModelFile(context, "DeepFilterNet3_ll_onnx.tgz");
deepFilterNet = new DeepFilterNet(modelBytes);
```

### 参数说明

#### `initialize(postFilterBeta, attenLimDb)`

- **postFilterBeta** (float): 后滤波器 beta 参数
  - 控制降噪强度
  - > 0 启用后滤波
  - 推荐值：0.5 - 2.0
  - 值越大，降噪越强

- **attenLimDb** (float): 衰减限制（dB）
  - 控制最大降噪幅度
  - 推荐值：20.0 - 40.0
  - 值越大，允许的最大降噪幅度越大

#### 返回值 LSNR

- **LSNR** (Log-Signal-to-Noise Ratio): 对数信噪比
  - 正数：处理成功，值越大表示降噪效果越好
  - 负数：处理失败

## GitHub Actions 自动构建

本项目使用 GitHub Actions 自动构建和发布：

1. 推送标签（如 `v1.0.0`）到仓库
2. GitHub Actions 自动触发构建流程
3. 构建完成后自动创建 Release
4. Release 包含以下文件：
   - `deepfilter-release.aar`: AAR 包
   - `libdeepfilter_ort.so`: 动态库
   - `df.h`: 头文件
   - `VERSION.txt`: 版本信息

## 原生开发

如果你想在原生 C/C++ 代码中使用 DeepFilterNet，可以使用提供的头文件和动态库：

```c
#include "df.h"

// 创建实例
void* df = df_create(model_bytes, model_size, 0.5f, 30.0f);

// 处理音频
float input[frame_size];
float output[frame_size];
float lsnr = df_process_frame(df, input, output, frame_size);

// 销毁实例
df_destroy(df);
```

## 技术架构

```
┌─────────────────────────────────────┐
│         Android Application         │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      DeepFilterNet.java             │
│         (JNI 接口)                  │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      native-lib.cpp                 │
│         (JNI 实现)                   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      libdeepfilter_ort.so           │
│         (Rust 动态库)                │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      DeepFilterNet Core             │
│         (降噪算法)                   │
└─────────────────────────────────────┘
```

## 性能优化建议

1. **帧大小选择**：建议使用 512 或 1024 采样点的帧大小
2. **采样率**：支持 16kHz 或 48kHz 采样率
3. **内存管理**：及时调用 `release()` 释放资源
4. **模型加载**：建议在应用启动时加载模型，避免重复加载

## 常见问题

### Q: 支持哪些架构？

A: 目前仅支持 ARM64 架构（arm64-v8a）。

### Q: 模型文件从哪里获取？

A: 需要自行准备 DeepFilterNet3 模型文件（tar.gz 格式），可以从 [DeepFilterNet 官方仓库](https://github.com/Rikorose/DeepFilterNet)获取。

### Q: 支持立体声音频吗？

A: 目前仅支持单声道音频。如需处理立体声，请分别处理左右声道。

### Q: 如何调整降噪强度？

A: 通过调整 `initialize()` 方法的 `postFilterBeta` 和 `attenLimDb` 参数来控制降噪强度。

## 许可证

本项目遵循 DeepFilterNet 原项目的许可证（Apache-2.0 和 MIT）。

## 致谢

- [DeepFilterNet](https://github.com/Rikorose/DeepFilterNet) - 原始降噪算法实现
- [Tract](https://github.com/snipsco/tract) - ONNX 推理框架

## 联系方式

如有问题或建议，请提交 [Issue](../../issues)。

## 更新日志

### v1.0.0
- 初始版本
- 支持 ARM64 架构
- 提供完整的 JNI 接口
- 支持实时音频降噪
