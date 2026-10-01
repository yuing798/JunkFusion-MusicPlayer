---
不要往下看，这个Readme是随便写的，现在项目还在施工中
---





webView2 安装:运行 Register-PackageSource -provider NuGet -name nugetRepository -location https://www.nuget.org/api/v2
Install-Package Microsoft.Web.WebView2 -Scope CurrentUser -RequiredVersion 1.0.3485.44 -Source nugetRepository


## 开发环境启动指南 (Development Guide)

本项目采用 **Vue 3 (前端) + JUCE / CMake (C++ 后端)** 分离架构。开发时，**前端热重载**和**C++ 调试**需要同时进行。

请按照以下步骤，**打开两个独立的终端窗口（Terminal）**：

### 1. 终端 A：启动前端开发服务器（Vue）
在项目根目录下执行：
```bash
cd UI                  # 进入前端目录
npm install            # （仅第一次克隆时执行）安装依赖
npm run dev            # 启动 Vite 开发服务器（会保持运行，不要关闭此终端）
```
> **保持此终端运行**。启动成功后，你会在终端看到 `Local: http://localhost:5173/` 等地址。修改 Vue 代码后，此终端会自动热重载。

### 2. 终端 B：配置并编译 C++ (JUCE)
回到项目根目录（**新开一个终端窗口**）：
```bash
# 第一步：生成 CMake 构建文件（只需在首次或修改 CMakeLists.txt 后执行一次）
cmake --presets <你的预设名称，比如 vs2022-debug>  

# 第二步：编译 C++ 工程（每次修改 C++ 代码后执行）
cmake --build --preset <你的预设名称> --config Debug
```

### 💡 开发时的联动逻辑
- **只改 Vue 代码**：终端 A 的 `npm run dev` 会自动刷新页面，**无需重新编译 C++**。
- **只改 C++ 代码**：在终端 B 重新执行 `cmake --build ...` 即可。
- **改了 `UI/bridgeDefs.json`（通信协议）**：
  1. 终端 A 的 `npm run dev` 会自动检测 `json` 变化，重新生成 TS 和 `BridgeNames.h`（取决于你的 `generate` 脚本）。
  2. 终端 B 需**重新编译** C++，因为 `BridgeNames.h` 更新了。

## 编译 C++ 后端 (JUCE)

### 情况 1：使用 VSCode + CMake Tools 插件（推荐，最简单）
1. 安装插件：`ms-vscode.cmake-tools`。
2. 打开项目根目录，VSCode 右下角会提示“扫描工具包”，选择 **Visual Studio 2022 (amd64)** 即可。
3. 按 `F7`（或点击底部状态栏的 **Build** 按钮）直接编译。

> 💡 开发时：C++ 修改后按 `F7` 增量编译；Vue 修改后前端自动热重载，无需重编 C++。

---

### 情况 2：不使用 VSCode 插件（纯终端 / CLion 等）
**Windows 环境（MSVC）：**

请确保已安装 **Visual Studio 2022** 并勾选“使用 C++ 的桌面开发”组件。

在项目根目录打开 **任意终端（CMD/PowerShell）**，执行以下命令（无需额外配置环境变量）：

```bash
# 第一步：生成 Visual Studio 解决方案（只需执行一次）
cmake -B build -G "Visual Studio 17 2022" -A x64

# 第二步：编译工程（每次修改 C++ 后执行）
cmake --build build --config Debug
```

> 🔍 如果你更喜欢用 Ninja 或 Makefile，请确保你在 **“VS 2022 开发人员命令提示符”** 中打开终端，否则会找不到 `cl.exe`。

**macOS / Linux 环境：**
直接使用系统默认编译器（Clang/GCC）：
```bash
cmake -B build
cmake --build build
```

## 🖥️ 平台支持 (Platform Support)

- ✅ **Windows (x64)**：完全支持。日常开发和测试环境（MSVC）。
- ❓ **macOS / Linux**：CMake 构建脚本已配置，理论上支持，但**尚未经过完整测试**。

## 🍏 macOS / Linux 用户快速尝试 (Community Support)

由于我没有 macOS 真机环境，以下步骤**仅供参考**，如果你遇到问题，欢迎提 Issue 补充解决方案。

1. 安装依赖（通常系统自带 Clang，只需确保安装 CMake 和 Node.js）：
   ```bash
   brew install cmake node  # macOS
   # sudo apt install cmake nodejs npm  # Ubuntu/Debian
   ```

2. 构建 C++ 后端（在项目根目录执行）：
   ```bash
   # CMake 在 macOS/Linux 下默认会使用系统的 Clang/GCC，无需指定编译器
   cmake -B build
   cmake --build build --config Debug
   ```

3. 前端部分与 Windows 完全一致（Node.js 是跨平台的）：
   ```bash
   cd UI && npm install && npm run dev
   ```

> ⚠️ **注意**：如果编译报错，可能是 JUCE 依赖的音频/图形库路径不同，请参考 [JUCE 官方 Linux/macOS 编译指南](https://juce.com/learn/tutorials)。

juce 8 不适合windows10以下的版本
到时候没有私自测试过的环境不要写进readme里面