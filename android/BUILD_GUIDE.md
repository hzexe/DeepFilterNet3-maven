# 构建说明

## 快速开始

### 构建 Release 版本

```bash
cd android
gradlew.bat :deepfilter:assembleRelease
```

### 构建 Debug 版本

```bash
cd android
gradlew.bat :deepfilter:assembleDebug
```

## 构建流程

Gradle 会自动执行以下步骤：

1. **编译 Rust 项目**
   - 编译 `deepfilter-ort` 项目
   - 生成 `libdeepfilter_ort.so` 动态库

2. **复制 .so 文件**
   - 将 `.so` 文件复制到 `deepfilter/src/main/jniLibs/arm64-v8a/` 目录
   - Android 构建系统会自动将其打包到 AAR 中

3. **构建 AAR 包**
   - 编译 Java 代码
   - 打包所有资源（包括 .so 文件）
   - 生成 AAR 文件

## 输出文件

### Release 版本
- **位置**: `deepfilter/build/outputs/aar/deepfilter-release.aar`
- **内容**: 包含编译过的 Java 类和 Rust 动态库

### Debug 版本
- **位置**: `deepfilter/build/outputs/aar/deepfilter-debug.aar`
- **内容**: 包含编译过的 Java 类和 Rust 动态库（未优化）

## 手动编译 Rust 项目

如果只想编译 Rust 项目而不构建 AAR：

```bash
cd android
gradlew.bat compileNative
```

这将编译 `deepfilter-ort` 项目并复制 `.so` 文件到 `jniLibs` 目录。

## 环境要求

### 必需工具

- [Android Studio](https://developer.android.com/studio)（包含 Android SDK 和 NDK）
- [Rust](https://www.rust-lang.org/tools/install)
- Gradle（项目已包含 Gradle wrapper）

### 环境变量配置

#### 1. 设置 Android NDK 路径

**Windows**:
```cmd
set ANDROID_NDK_HOME=C:\path\to\ndk
```

**Linux/macOS**:
```bash
export ANDROID_NDK_HOME=/path/to/ndk
```

#### 2. 安装 Android NDK

如果尚未安装 NDK，可以通过 Android Studio 安装：

1. 打开 Android Studio
2. 进入 **Settings** → **Appearance & Behavior** → **System Settings** → **Android SDK**
3. 选择 **SDK Tools** 选项卡
4. 勾选 **NDK (Side by side)** 并点击 **Apply**

#### 3. 安装 Rust Android 目标

Gradle 会自动检查并安装 Rust Android 目标，但也可以手动安装：

```bash
rustup target add aarch64-linux-android
```

#### 4. 验证环境

验证 NDK 是否正确设置：

**Windows**:
```cmd
echo %ANDROID_NDK_HOME%
```

**Linux/macOS**:
```bash
echo $ANDROID_NDK_HOME
```

验证 Rust Android 目标是否已安装：

```bash
rustup target list --installed
```

### 交叉编译说明

Gradle 的 `compileNative` 任务会自动处理以下内容：

1. **检查 Rust Android 目标**
   - 自动检查 `aarch64-linux-android` 目标是否已安装
   - 如果未安装，自动执行 `rustup target add aarch64-linux-android`

2. **检查 Android NDK**
   - 检查 `ANDROID_NDK_HOME` 或 `NDK_HOME` 环境变量
   - 验证 NDK 路径是否存在
   - 如果未设置，提供清晰的设置说明

3. **检查交叉编译工具链**
   - 检查 NDK 工具链目录
   - 自动检测主机操作系统（Windows/Linux/macOS）
   - 选择适合的预构建工具链

4. **设置交叉编译环境变量**
   - 自动设置 `CC_aarch64_linux_android`
   - 自动设置 `CXX_aarch64_linux_android`
   - 自动设置 `AR_aarch64_linux_android`
   - 自动设置 `CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER`

详细的交叉编译配置说明请参考：[CROSS_COMPILE_ENV.md](CROSS_COMPILE_ENV.md)

## 验证 AAR 内容

可以使用以下命令验证 AAR 包的内容：

```powershell
# 解压 AAR 包
$tempDir = "temp_aar_extract"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
Expand-Archive -Path "deepfilter/build/outputs/aar/deepfilter-release.aar" -DestinationPath $tempDir -Force

# 查看内容
Get-ChildItem -Path $tempDir -Recurse | Select-Object FullName | Format-Table -AutoSize

# 清理临时目录
Remove-Item -Path $tempDir -Recurse -Force
```

## 常见问题

### Q: 为什么不需要手动复制 .so 文件？

A: Gradle 构建系统会自动将 `jniLibs` 目录中的 `.so` 文件打包到 AAR 中，无需手动操作。

### Q: 如何只编译 Rust 项目？

A: 运行 `gradlew.bat compileNative` 命令。

### Q: 支持 Debug 和 Release 版本吗？

A: 是的，支持 `assembleDebug` 和 `assembleRelease` 两种构建类型。

### Q: .so 文件在哪里？

A: 编译后的 .so 文件位于 `deepfilter/src/main/jniLibs/arm64-v8a/libdeepfilter_ort.so`，并已打包到 AAR 中。
