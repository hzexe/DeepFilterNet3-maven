# Android音频录制与降噪集成使用指南

## 概述

本项目实现了完整的Android音频录制与降噪功能，将AAudio音频录制和DeepFilterNet降噪处理完全集成在Native层，提供高效的实时音频处理能力。

## 主要特性

1. **完全集成**：录制和降噪在Native层完成，无需Java层干预
2. **异步处理**：使用独立线程进行降噪处理，避免阻塞音频采集线程
3. **队列缓存**：使用队列缓存音频数据，确保实时处理能力
4. **固定格式**：音频格式固定为48kHz、单声道、PCM_FLOAT
5. **实时处理**：支持实时音频流处理，延迟低
6. **参数可配**：支持动态配置降噪参数（post_filter_beta、atten_lim_db）
7. **性能优化**：使用AAudio低延迟模式，确保实时处理能力
8. **错误恢复**：队列满时自动丢弃最旧的帧，防止内存溢出

## 项目结构

```
android/deepfilter/src/main/
├── cpp/
│   ├── CMakeLists.txt                    # CMake构建配置
│   ├── include/
│   │   └── AudioProcessor.h             # 音频处理器头文件
│   └── src/
│       ├── AudioProcessor.cpp             # 音频处理器实现（集成录制和降噪）
│       └── jni_interface.cpp            # JNI接口实现
└── java/com/hzexe/audio/ns/
    ├── AudioProcessor.java               # 音频处理器Java类
    ├── AudioProcessorTest.java           # 测试类
    └── DeepFilterNet.java             # 原有DeepFilterNet类（保留）
```

## 技术架构

```
┌─────────────────────────────────────┐
│         Android Application         │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      AudioProcessor.java            │
│         (JNI 接口)                │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      jni_interface.cpp             │
│         (JNI 实现)                 │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      AudioProcessor.cpp            │
│    (录制 + 降噪集成)              │
└──────┬──────────────────┬─────────┘
       │                  │
┌──────▼──────────┐   ┌───▼──────────────────┐
│   AAudio API    │   │  deepfilter-ort.so   │
│   (音频录制)     │   │    (降噪处理)         │
└─────────────────┘   └──────────────────────┘
       │
┌──────▼──────────────────────────┐
│   音频数据队列 (Queue)           │
│   - 缓存音频帧                   │
│   - 防止阻塞采集线程             │
│   - 最大10帧                    │
└──────────────────────────────────┘
       │
┌──────▼──────────────────────────┐
│   异步处理线程                   │
│   - 从队列取数据                 │
│   - 调用降噪处理                 │
│   - 返回结果给Java层             │
└──────────────────────────────────┘
```

## 快速开始

### 1. 基本使用

```java
import com.hzexe.audio.ns.AudioProcessor;

public class AudioProcessorExample {
    private AudioProcessor audioProcessor;
    
    // 初始化
    public void init(byte[] modelBytes) {
        audioProcessor = new AudioProcessor();
        
        // 初始化音频处理器
        // postFilterBeta: 后滤波器beta参数（控制降噪强度）
        // attenLimDb: 衰减限制（dB）
        boolean success = audioProcessor.initialize(modelBytes, 0.5f, 30.0f);
        
        if (!success) {
            throw new RuntimeException("AudioProcessor初始化失败");
        }
    }
    
    // 开始录制和降噪处理
    public void startProcessing() {
        audioProcessor.start(new AudioProcessor.AudioDataCallback() {
            @Override
            public void onAudioData(float[] audioData, float numFrames, float lsnr) {
                // audioData: 降噪后的音频数据（f32格式，单声道）
                // numFrames: 帧数
                // lsnr: LSNR值（信噪比）
                
                // 处理降噪后的音频数据
                processAudio(audioData);
            }
        });
    }
    
    // 停止处理
    public void stopProcessing() {
        audioProcessor.stop();
    }
    
    // 释放资源
    public void release() {
        if (audioProcessor != null) {
            audioProcessor.release();
            audioProcessor = null;
        }
    }
    
    private void processAudio(float[] audioData) {
        // 处理音频数据，例如播放、保存等
    }
}
```

### 2. 加载模型文件

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
audioProcessor = new AudioProcessor(modelBytes);
```

### 3. 动态调整降噪参数

```java
// 设置后滤波器beta参数（控制降噪强度）
// 推荐值：0.5 - 2.0
// 值越大，降噪越强
audioProcessor.setPostFilterBeta(1.0f);

// 设置衰减限制（dB）
// 推荐值：20.0 - 40.0
// 值越大，允许的最大降噪幅度越大
audioProcessor.setAttenLimDb(35.0f);
```

### 4. 监控队列状态

```java
// 获取当前队列中的帧数
int queueSize = audioProcessor.getQueueSize();

// 队列大小范围：0-10
// 如果队列经常接近10，说明处理速度跟不上采集速度
// 可能需要优化降噪参数或使用更快的设备
if (queueSize > 8) {
    Log.w("AudioProcessor", "队列接近满载，可能存在处理延迟");
}
```

## 参数说明

### initialize(tarBytes, postFilterBeta, attenLimDb)

- **tarBytes** (byte[]): 模型文件字节数组（tar.gz格式）
- **postFilterBeta** (float): 后滤波器beta参数
  - 控制降噪强度
  - > 0 启用后滤波
  - 推荐值：0.5 - 2.0
  - 值越大，降噪越强

- **attenLimDb** (float): 衰减限制（dB）
  - 控制最大降噪幅度
  - 推荐值：20.0 - 40.0
  - 值越大，允许的最大降噪幅度越大

### onAudioData(audioData, numFrames, lsnr)

- **audioData** (float[]): 降噪后的音频数据（f32格式，单声道）
- **numFrames** (float): 帧数
- **lsnr** (float): LSNR值（Log-Signal-to-Noise Ratio）
  - 正数：处理成功，值越大表示降噪效果越好
  - 负数：处理失败

## 音频格式

- **采样率**: 48000 Hz（固定）
- **声道数**: 1（单声道，固定）
- **位深度**: 32-bit Float（PCM_FLOAT）
- **帧大小**: 512 或 1024（根据模型配置）
- **字节序**: Little-Endian（小端序）
- **浮点标准**: IEEE 754

**注意**：由于C++和Rust代码运行在同一个ARM64设备上，使用相同的字节序（Little-Endian）和相同的浮点标准（IEEE 754），因此不需要任何字节序转换。数据可以直接传递和处理。

详细分析请参考：[ENDIANNESS_ANALYSIS.md](file:///F:/DeepFilterNet3-maven/android/ENDIANNESS_ANALYSIS.md)

## 性能优化建议

1. **帧大小选择**：建议使用512或1024采样点的帧大小
2. **内存管理**：及时调用`release()`释放资源
3. **模型加载**：建议在应用启动时加载模型，避免重复加载
4. **参数调整**：根据实际场景调整降噪参数

## 测试

使用提供的测试类验证功能：

```java
import com.hzexe.audio.ns.AudioProcessorTest;

public class MainActivity extends AppCompatActivity {
    private AudioProcessorTest test;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        test = new AudioProcessorTest();
        
        // 运行完整测试（测试10秒）
        boolean success = test.testFullProcess(this, 10000);
        
        if (success) {
            Log.d("Test", "所有测试通过");
        } else {
            Log.e("Test", "测试失败");
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (test != null) {
            test.release();
        }
    }
}
```

## 构建说明

### 编译顺序

1. **编译Rust项目**（生成deepfilter-ort.so）
   ```bash
   cd android
   gradlew.bat compileNative
   ```

2. **编译C++项目**（生成deepfilter_native.so）
   ```bash
   gradlew.bat :deepfilter:externalNativeBuildRelease
   ```

3. **构建AAR包**
   ```bash
   gradlew.bat :deepfilter:assembleRelease
   ```

### 输出文件

- **deepfilter-ort.so**: `deepfilter/src/main/jniLibs/arm64-v8a/libdeepfilter_ort.so`
- **deepfilter_native.so**: `deepfilter/build/intermediates/cmake/release/obj/arm64-v8a/libdeepfilter_native.so`
- **AAR包**: `deepfilter/build/outputs/aar/deepfilter-release.aar`

## 常见问题

### Q: 为什么音频格式是固定的？

A: 为了确保与DeepFilterNet模型的兼容性，音频格式被固定为48kHz、单声道、PCM_FLOAT。这样可以避免格式转换，提高处理效率。

### Q: 如何调整降噪强度？

A: 通过调整`initialize()`方法的`postFilterBeta`和`attenLimDb`参数来控制降噪强度。

### Q: 支持立体声音频吗？

A: 目前仅支持单声道音频。如需处理立体声，请分别处理左右声道。

### Q: 如何验证降噪效果？

A: 可以通过LSNR值判断降噪效果，LSNR值越大表示降噪效果越好。也可以通过播放降噪前后的音频进行对比。

### Q: 内存占用如何？

A: 音频处理器本身占用内存较小，主要内存消耗来自模型加载。建议在应用启动时加载模型，避免重复加载。

## 技术细节

### 异步处理架构

**为什么需要异步处理？**

AAudio的dataCallback在音频线程中执行，对时间非常敏感。如果在这个回调中直接进行降噪处理（包含FFT、神经网络推理等耗时操作），会导致：

1. **音频缓冲区溢出**：处理时间超过音频帧间隔时间
2. **丢帧**：音频数据来不及处理就被覆盖
3. **音频卡顿**：播放/录制出现断裂
4. **延迟增加**：实时性严重下降

**异步处理方案：**

```
音频采集线程（AAudio）      处理线程（独立）
    │                           │
    ├─ dataCallback             ├─ processingThreadFunc
    │  ├─ 快速复制音频数据      │  ├─ 从队列取数据
    │  ├─ 放入队列              │  ├─ 调用降噪处理
    │  ├─ 通知处理线程          │  ├─ 返回结果给Java层
    │  └─ 立即返回（<1ms）      │  └─ 处理耗时（10-20ms）
    │                           │
```

**队列管理：**

- **队列大小**：最大10帧（约100-200ms音频）
- **队列满时**：自动丢弃最旧的帧，防止内存溢出
- **队列空时**：处理线程等待，避免CPU空转
- **线程同步**：使用mutex和condition_variable确保线程安全

**性能优势：**

1. **零阻塞**：AAudio回调只做快速复制（<1ms），立即返回
2. **实时性**：音频采集不受降噪处理速度影响
3. **稳定性**：即使降噪处理较慢，也不会影响音频采集
4. **可扩展**：可以轻松添加多个处理线程

### Native层集成

AudioProcessor.cpp实现了录制和降噪的完整流程：

1. **AAudio录制**：使用AAudio API进行低延迟音频录制
2. **数据回调**：在AAudio数据回调中快速将数据放入队列
3. **异步处理**：独立线程从队列取数据进行降噪
4. **降噪处理**：调用deepfilter-ort库进行实时降噪
5. **结果输出**：通过JNI回调将降噪后的音频数据返回给Java层

### 错误处理

- AAudio初始化失败
- DeepFilterNet模型加载失败
- 参数非法（负数等）
- 内存分配失败

所有错误都会通过日志输出，并可通过`getLastError()`获取详细错误信息。

## 许可证

本项目遵循DeepFilterNet原项目的许可证（Apache-2.0和MIT）。

## 更新日志

### v2.1
- 重构为异步处理架构，避免阻塞音频采集线程
- 添加音频数据队列，最大10帧缓存
- 添加独立处理线程进行降噪处理
- 添加队列状态监控接口（getQueueSize）
- 队列满时自动丢弃最旧的帧，防止内存溢出
- 优化AAudio回调性能，只做快速复制（<1ms）

### v2.0
- 重构AudioProcessor，集成录制和降噪功能
- 音频格式固定为48kHz、单声道、PCM_FLOAT
- 简化JNI接口，提供更清晰的API
- 添加完整的测试用例

### v1.0
- 初始版本
- 支持ARM64架构
- 提供完整的JNI接口
- 支持实时音频降噪