# 快速开始指南

## 5 分钟快速上手 DeepFilterNet Android 库

### 步骤 1：获取 AAR 包

有两种方式获取 AAR 包：

#### 方式 A：从 GitHub Release 下载（推荐）

1. 访问项目的 [Releases 页面](../../releases)
2. 下载最新版本的 `deepfilter-release.aar`

#### 方式 B：自行构建

参考 [BUILD_GUIDE.md](BUILD_GUIDE.md) 进行本地构建。

### 步骤 2：集成到 Android 项目

1. **创建 libs 目录**（如果不存在）：
   ```
   your-app/
   ├── app/
   │   ├── libs/              # 创建此目录
   │   └── src/
   ```

2. **复制 AAR 文件**：
   - 将 `deepfilter-release.aar` 复制到 `app/libs/` 目录

3. **修改 build.gradle**：
   
   在 `app/build.gradle` 中添加：
   ```gradle
   dependencies {
       implementation(fileTree(dir: "libs", include: ["*.aar"]))
   }
   ```

### 步骤 3：准备模型文件

1. **下载模型**：
   - 从 [DeepFilterNet 官方仓库](https://github.com/Rikorose/DeepFilterNet) 下载模型
   - 推荐使用 `DeepFilterNet3_ll_onnx.tgz`

2. **放置模型文件**：
   - 将模型文件复制到 `app/src/main/assets/` 目录
   - 或从应用外部加载（推荐，避免 APK 体积过大）

### 步骤 4：编写代码

#### 简单示例

```java
import com.hzexe.audio.ns.DeepFilterNet;

public class MainActivity extends AppCompatActivity {
    private DeepFilterNet deepFilterNet;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 1. 加载模型文件
        byte[] modelBytes = loadModelFile("DeepFilterNet3_ll_onnx.tgz");
        
        // 2. 创建实例
        deepFilterNet = new DeepFilterNet(modelBytes);
        
        // 3. 初始化
        boolean success = deepFilterNet.initialize(0.5f, 30.0f);
        
        if (!success) {
            Log.e("MainActivity", "初始化失败");
            return;
        }
        
        // 4. 处理音频
        float[] input = getAudioInput(); // 获取音频输入
        float[] output = new float[input.length];
        float lsnr = deepFilterNet.process(input, output);
        
        if (lsnr >= 0) {
            Log.d("MainActivity", "处理成功，LSNR: " + lsnr);
            // 使用 output 数据
        }
    }
    
    private byte[] loadModelFile(String assetPath) {
        try {
            InputStream is = getAssets().open(assetPath);
            ByteArrayOutputStream buffer = new ByteArrayOutputStream();
            
            byte[] data = new byte[16384];
            int nRead;
            
            while ((nRead = is.read(data, 0, data.length)) != -1) {
                buffer.write(data, 0, nRead);
            }
            
            buffer.flush();
            is.close();
            
            return buffer.toByteArray();
        } catch (IOException e) {
            Log.e("MainActivity", "加载模型失败", e);
            return null;
        }
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        // 5. 释放资源
        if (deepFilterNet != null) {
            deepFilterNet.release();
        }
    }
}
```

### 步骤 5：配置参数

#### 初始化参数

```java
// 初始化时设置参数
deepFilterNet.initialize(postFilterBeta, attenLimDb);
```

**参数说明**：

| 参数 | 类型 | 范围 | 说明 | 推荐值 |
|------|------|------|------|--------|
| postFilterBeta | float | > 0 | 后滤波器 beta 参数，控制降噪强度 | 0.5 - 2.0 |
| attenLimDb | float | 0 - 100 | 衰减限制（dB），控制最大降噪幅度 | 20.0 - 40.0 |

**参数调优建议**：

- **轻度降噪**：`postFilterBeta = 0.5`, `attenLimDb = 20.0`
- **中度降噪**：`postFilterBeta = 1.0`, `attenLimDb = 30.0`
- **重度降噪**：`postFilterBeta = 2.0`, `attenLimDb = 40.0`

### 步骤 6：处理音频

#### 单帧处理

```java
float[] input = new float[512]; // 帧大小
float[] output = new float[input.length];
float lsnr = deepFilterNet.process(input, output);
```

#### 批量处理

```java
float[] longInput = new float[512 * 100]; // 100 帧
float[] longOutput = new float[longInput.length];

for (int i = 0; i < 100; i++) {
    int offset = i * 512;
    float[] frameInput = new float[512];
    float[] frameOutput = new float[512];
    
    System.arraycopy(longInput, offset, frameInput, 0, 512);
    float lsnr = deepFilterNet.process(frameInput, frameOutput);
    
    if (lsnr >= 0) {
        System.arraycopy(frameOutput, 0, longOutput, offset, 512);
    }
}
```

#### 实时处理

```java
// 在音频回调中处理
public void onAudioData(byte[] audioData) {
    byte[] output = new byte[audioData.length];
    float lsnr = deepFilterNet.process(audioData, output);
    
    if (lsnr >= 0) {
        // 发送处理后的音频
        sendProcessedAudio(output);
    }
}
```

### 常见问题

#### Q: LSNR 是什么？

A: LSNR（Log-Signal-to-Noise Ratio）是对数信噪比，用于衡量降噪效果：
- 正数：处理成功，值越大表示降噪效果越好
- 负数：处理失败

#### Q: 帧大小应该设置多少？

A: 推荐使用 512 或 1024 采样点：
- 512：延迟更低，适合实时处理
- 1024：降噪效果更好，延迟稍高

#### Q: 支持哪些音频格式？

A: 支持 f32（32位浮点）格式的单声道音频：
- 采样率：16kHz 或 48kHz
- 声道：单声道
- 格式：f32（小端序）

#### Q: 如何处理立体声音频？

A: 目前仅支持单声道。如需处理立体声，请分别处理左右声道：

```java
// 分离左右声道
float[] leftChannel = extractLeftChannel(stereoAudio);
float[] rightChannel = extractRightChannel(stereoAudio);

// 分别处理
float[] processedLeft = df.processAudio(leftChannel);
float[] processedRight = df.processAudio(rightChannel);

// 合并左右声道
float[] processedStereo = mergeChannels(processedLeft, processedRight);
```

#### Q: 模型文件太大怎么办？

A: 建议从应用外部加载模型：

```java
// 从外部存储加载
File modelFile = new File(getExternalFilesDir(null), "DeepFilterNet3_ll_onnx.tgz");
byte[] modelBytes = Files.readAllBytes(modelFile.toPath());
```

### 性能优化建议

1. **重用实例**：不要频繁创建和销毁 DeepFilterNet 实例
2. **批量处理**：使用批量处理减少 JNI 调用次数
3. **内存管理**：及时释放不需要的音频数据
4. **线程安全**：DeepFilterNet 不是线程安全的，确保在单线程中使用

### 下一步

- 查看 [README.md](README.md) 了解更多详细信息
- 查看 [DeepFilterNetExample.java](deepfilter/src/main/java/com/hzexe/audio/ns/example/DeepFilterNetExample.java) 获取完整示例
- 查看 [BUILD_GUIDE.md](BUILD_GUIDE.md) 了解如何自行构建

### 获取帮助

如有问题，请：
1. 查看 [README.md](README.md) 常见问题部分
2. 提交 [Issue](../../issues)
3. 查看 [DeepFilterNet 官方文档](https://github.com/Rikorose/DeepFilterNet)
