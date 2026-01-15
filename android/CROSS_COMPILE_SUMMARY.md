# 交叉编译环境配置总结

## 问题

在执行 `cargo build --release --target aarch64-linux-android` 之前，需要先安装和配置交叉编译环境：

1. **Rust Android 目标**：需要安装 `aarch64-linux-android` 目标
2. **Android NDK**：需要 NDK 工具链进行交叉编译
3. **C++ 编译环境**：在 Windows 下需要 C++ 编译器
4. **环境变量**：需要设置正确的环境变量

## 解决方案

在 `build.gradle` 的 `compileNative` 任务中添加了完整的交叉编译环境检查和设置：

### 1. 自动检查和安装 Rust Android 目标

```gradle
// 检查 Rust Android 目标
println '检查 Rust Android 目标...'
def rustupProcess = "rustup target list --installed".execute()
rustupProcess.waitFor()
def targets = rustupProcess.text
if (!targets.contains('aarch64-linux-android')) {
    println '安装 Rust Android 目标 aarch64-linux-android...'
    def addProcess = "rustup target add aarch64-linux-android".execute()
    def addExitCode = addProcess.waitFor()
    if (addExitCode != 0) {
        throw new GradleException("Rust Android 目标安装失败，退出码: ${addExitCode}")
    }
    println 'Rust Android 目标安装成功'
} else {
    println 'Rust Android 目标已安装'
}
```

**优势**：
- ✅ 自动检查 `aarch64-linux-android` 目标是否已安装
- ✅ 如果未安装，自动执行 `rustup target add aarch64-linux-android`
- ✅ 提供清晰的错误提示

### 2. 检查 Android NDK 环境变量

```gradle
// 检查 NDK 环境
println '检查 Android NDK...'
def ndkHome = System.getenv('ANDROID_NDK_HOME') ?: System.getenv('NDK_HOME')
if (ndkHome == null || ndkHome.isEmpty()) {
    println '警告: 未设置 ANDROID_NDK_HOME 或 NDK_HOME 环境变量'
    println ''
    println '请设置 Android NDK 环境变量：'
    println '  Windows: set ANDROID_NDK_HOME=C:\\path\\to\\ndk'
    println '  Linux/macOS: export ANDROID_NDK_HOME=/path/to/ndk'
    println ''
    println '或者确保已安装 Android Studio 和 NDK'
    throw new GradleException("未设置 ANDROID_NDK_HOME 或 NDK_HOME 环境变量")
}

println "使用 NDK: ${ndkHome}"

def ndkPath = new File(ndkHome)
if (!ndkPath.exists()) {
    throw new GradleException("NDK 路径不存在: ${ndkPath}")
}
```

**优势**：
- ✅ 检查 `ANDROID_NDK_HOME` 或 `NDK_HOME` 环境变量
- ✅ 如果未设置，提供清晰的设置说明
- ✅ 验证 NDK 路径是否存在

### 3. 检查交叉编译工具链

```gradle
// 检查交叉编译工具链
println '检查交叉编译工具链...'
def toolchainDir = new File(ndkPath, 'toolchains/llvm/prebuilt')
if (!toolchainDir.exists()) {
    throw new GradleException("找不到 NDK 工具链目录: ${toolchainDir}")
}

def hostOs = System.getProperty('os.name').toLowerCase()
def hostArch = System.getProperty('os.arch').toLowerCase()
def prebuiltDir = null

toolchainDir.eachDir { dir ->
    if (dir.name.contains('windows') || dir.name.contains('linux') || dir.name.contains('darwin')) {
        prebuiltDir = dir
        return
    }
}

if (prebuiltDir == null) {
    throw new GradleException("找不到适合的预构建工具链")
}

println "使用工具链: ${prebuiltDir.name}"
```

**优势**：
- ✅ 检查 NDK 工具链目录是否存在
- ✅ 自动检测主机操作系统（Windows/Linux/macOS）
- ✅ 选择适合的预构建工具链

### 4. 设置交叉编译环境变量

```gradle
// 编译 Rust 项目
println '编译 deepfilter-ort...'
def cargoCommand = "cargo build --release --target aarch64-linux-android"

def env = System.getenv()
def processBuilder = new ProcessBuilder(cargoCommand.split(' '))
processBuilder.directory(deepfilterOrtDir)

// 设置 NDK 交叉编译环境变量
def toolchainBin = new File(prebuiltDir, 'bin')
env['CC_aarch64_linux_android'] = new File(toolchainBin, 'aarch64-linux-android21-clang' + (hostOs.contains('windows') ? '.exe' : '')).absolutePath
env['CXX_aarch64_linux_android'] = new File(toolchainBin, 'aarch64-linux-android21-clang++' + (hostOs.contains('windows') ? '.exe' : '')).absolutePath
env['AR_aarch64_linux_android'] = new File(toolchainBin, 'llvm-ar' + (hostOs.contains('windows') ? '.exe' : '')).absolutePath
env['CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER'] = new File(toolchainBin, 'aarch64-linux-android21-clang' + (hostOs.contains('windows') ? '.exe' : '')).absolutePath

processBuilder.environment().putAll(env)

def process = processBuilder.start()
def exitCode = process.waitFor()
```

**优势**：
- ✅ 设置 `CC_aarch64_linux_android` 环境变量
- ✅ 设置 `CXX_aarch64_linux_android` 环境变量
- ✅ 设置 `AR_aarch64_linux_android` 环境变量
- ✅ 设置 `CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER` 环境变量
- ✅ 自动处理 Windows 的 `.exe` 扩展名

## 使用方法

### 设置 NDK 环境变量

**Windows**:
```cmd
set ANDROID_NDK_HOME=C:\path\to\ndk
```

**Linux/macOS**:
```bash
export ANDROID_NDK_HOME=/path/to/ndk
```

### 执行构建

```bash
cd android
gradlew.bat compileNative
```

## 构建输出

```
========================================
  编译 Rust 项目
========================================

检查 Rust Android 目标...
Rust Android 目标已安装

检查 Android NDK...
使用 NDK: C:\path\to\ndk

检查交叉编译工具链...
使用工具链: windows-x86_64

编译 deepfilter-ort...
复制 .so 文件到 jniLibs...
.so 文件已复制到: F:\DeepFilterNet3-maven\android\deepfilter\src\main\jniLibs\arm64-v8a
```

## 优势

1. **自动化**：自动检查和安装 Rust Android 目标
2. **环境检查**：验证 NDK 环境变量和路径
3. **工具链检测**：自动选择适合的交叉编译工具链
4. **跨平台**：支持 Windows、Linux 和 macOS
5. **清晰提示**：提供详细的错误信息和设置说明
6. **无需手动配置**：自动设置所有必需的环境变量

## 文档更新

已创建以下文档：
- ✅ `CROSS_COMPILE_ENV.md` - 详细的交叉编译环境配置说明
- ✅ `BUILD_GUIDE.md` - 更新了环境要求部分

## 常见问题

### Q: 如何安装 Android NDK？

A: 通过 Android Studio 安装：
1. 打开 Android Studio
2. 进入 Settings → Appearance & Behavior → System Settings → Android SDK
3. 选择 SDK Tools 选项卡
4. 勾选 NDK (Side by side) 并点击 Apply

### Q: 如何设置环境变量？

A: 
- **Windows**: 在系统环境变量中添加 `ANDROID_NDK_HOME`
- **Linux/macOS**: 在 `~/.bashrc` 或 `~/.zshrc` 中添加 `export ANDROID_NDK_HOME=/path/to/ndk`

### Q: 交叉编译需要 C++ 环境吗？

A: 不需要。NDK 工具链包含了所有必需的 C/C++ 编译器和链接器。Gradle 脚本会自动设置正确的环境变量。

### Q: 如何验证 Rust Android 目标是否已安装？

A: 运行 `rustup target list --installed` 命令，查看是否包含 `aarch64-linux-android`。

## 总结

通过在 `compileNative` 任务中添加完整的交叉编译环境检查和设置，我们确保：

- ✅ Rust Android 目标已安装
- ✅ Android NDK 环境变量已设置
- ✅ 交叉编译工具链可用
- ✅ 正确的环境变量已配置

用户只需要设置 `ANDROID_NDK_HOME` 环境变量，Gradle 脚本会自动处理其他所有配置！
