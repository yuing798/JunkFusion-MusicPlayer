---
name: js-juce-bridge
description: This document describes how to implement communication between JavaScript and Juce.
---

# JUCE 8 WebView2 桥接文档

本文档说明 C++（JUCE 8）与 JS（Vue）之间如何通过 WebView2 进行双向通信。

---

## 一、架构概览

JUCE 8 的 WebView 双向通信基于 `window.__JUCE__` 全局对象。C++ 端在 WebView 初始化时注入一段底层 JS 脚本（`lowLevelIntegrationsScript`），该脚本创建 `window.__JUCE__.backend` 对象，提供事件收发机制。

```
┌─────────────────────────────────────────────────────────────┐
│                     Vue 前端 (JS/TS)                         │
│  ┌─────────────────────┐    ┌──────────────────────────────┐ │
│  │  initBridge.ts       │    │  juce-framework-frontend    │ │
│  │  (项目自定义封装)      │    │  (JUCE 官方 JS 库)           │ │
│  │  callJuceFunc()      │    │  getNativeFunction()         │ │
│  │                      │    │  getSliderState()            │ │
│  └──────────┬───────────┘    │  getToggleState()            │ │
│             │                │  getComboBoxState()          │ │
│             │                │  getBackendResourceAddress() │ │
│             │                └──────────────┬───────────────┘ │
│             │                               │                 │
│             └───────────┬───────────────────┘                 │
│                         ▼                                     │
│            window.__JUCE__.backend                             │
│            .addEventListener(eventId, fn)                      │
│            .emitEvent(eventId, object)                         │
│            .emitByBackend(eventId, object) ← C++ 调用用         │
└─────────────────────────┬───────────────────────────────────┘
                          │  postMessage / IPC
┌─────────────────────────┴───────────────────────────────────┐
│                    C++ 后端 (JUCE)                            │
│  ┌──────────────────────────────────────────────────────┐    │
│  │  WebBrowserComponent::Options                         │    │
│  │  .withNativeIntegrationEnabled()   //只有调用它后，前端的 window.__JUCE__.backend 对象才会激活                   │    │
│  │  .withNativeFunction(name, callback)   //在 C++ 后端注册一个原生函数，赋予前端通过 Promise 异步调用它的能力。               │    │
│  │  .withEventListener(eventId, listener)  //注册一个 C++ 监听器，专门用来接收前端主动单向过来的广播事件              │    │
.withResourceProvider(provider)//设置一个资源供应拦截器。当网页加载静态文件（如图片、CSS、JS）时，不走网络，而是直接去读取 C++ 的内存        │    │
│  │  .withOptionsFrom(relay) //把一个参数中继器（Relay，如 WebSliderRelay、WebToggleRelay）内部自带的配置批量融合进当前options中，                             │    │
│  │  .withUserScript(script)    //向 WebView 强行注入一段你自定义的 JavaScript 文本脚本，改脚本拥有最高特权                          │    │
│  │  .withInitialisationData(name, value) //往前端挂载一些在网页启动时就必须立马同步拿到的静态初始化数据                │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                                 │
│  emitEventIfBrowserIsVisible(eventId, object)                   │
│  evaluateJavascript(script, callback)                           │
└────────────────────────────────────────────────────────────────┘
```

---

## 二、`window.__JUCE__` 全局对象

这是 JUCE 8 在 WebView 中自动注入的全局命名空间。只要在 C++ 端调用了 `.withNativeIntegrationEnabled()`，页面加载时就会创建该对象。

### 2.1 结构

```javascript
window.__JUCE__ = {
  // 后端核心对象
  backend: {
    addEventListener(eventId, fn)  → [eventId, listenerId]  // 订阅事件
    removeEventListener([eventId, id])                       // 取消订阅
    emitEvent(eventId, object)      // 前端 → C++: 发送事件到后端
    emitByBackend(eventId, object)  // C++ → 前端: 后端推送事件（内部使用）
  },

  // 初始化数据（C++ 端通过 withInitialisationData 注入）
  initialisationData: {
    __juce__platform: [],          // 当前平台: "windows" | "macos" | "ios" | "android" | "linux"
    __juce__functions: [],          // 已注册的 NativeFunction 名称列表
    __juce__registeredGlobalEventIds: [],  // 全局注册的事件 ID
    __juce__sliders: [],            // WebSliderRelay 名称列表
    __juce__toggles: [],            // WebToggleButtonRelay 名称列表
    __juce__comboBoxes: [],         // WebComboBoxRelay 名称列表
  },

  postMessage: function() {},       // 平台相关的消息发送函数
}
```

### 2.2 环境检测

在普通浏览器（`npm run dev`）中没有 `window.__JUCE__`。必须在所有 JUCE API 调用前做检测：

```typescript
const isInsideJUCE = typeof window !== 'undefined' && window.__JUCE__ !== undefined
```

---

## 三、前端 → C++ 通信（JS 调用 C++ 函数）

### 3.1 底层机制

JUCE 8 的 `NativeFunction` 通过两个内部事件实现 Promise 式的异步调用：

| 事件 ID | 方向 | 用途 |
|---------|------|------|
| `__juce__invoke` | JS → C++ | 前端发起调用，携带 `{ name, params, resultId }` |
| `__juce__complete` | C++ → JS | 后端返回结果，携带 `{ promiseId, result }` |

### 3.2 使用 JUCE 官方 API（推荐用于新组件）

```javascript
import { getNativeFunction } from "juce-framework-frontend";

const myBackendFunction = getNativeFunction("myBackendFunction");
const result = await myBackendFunction(1, 2, "some string");
```

`getNativeFunction()` 返回一个 Promise 风格的函数，内部自动管理 `promiseId` 的分配和 Promise 的 resolve/reject。

### 3.3 项目自定义封装（当前使用）

本项目在 `UI/src/bridge/initBridge.ts` 中实现了自己的 Promise 管理层，相比官方的 `getNativeFunction` 多了**错误处理支持**：

```typescript
import { callJuceFunc } from "./bridge/initBridge.ts"
import { BRIDGE_KEYS } from "./bridge/bridge.generated.ts"

// 调用 C++ 函数，返回 Promise
const result = await callJuceFunc(BRIDGE_KEYS.inputFiles)
```

**错误处理机制：** C++ 端返回带有 `error` 属性的对象时，`initBridge.ts` 会自动 `reject` 该 Promise：

```cpp
// C++ 端：返回错误
auto error = new juce::DynamicObject();
error->setProperty("error", "文件插入失败");
complete(juce::var(error));
```

```typescript
// JS 端：通过 try/catch 捕获错误
try {
  await callJuceFunc(BRIDGE_KEYS.inputFiles)
} catch (e) {
  console.error(e.message) // "文件插入失败"
}
```

---

## 四、C++ → 前端通信（C++ 推送事件到 JS）

### 4.1 使用 EventListener（推荐）

**C++ 端**（在构造函数中配置）：

```cpp
// 注册事件监听器 — 前端向 C++ 发事件时触发
options = options.withEventListener("myEventId", [](const juce::var& payload) {
    // payload 是前端传来的数据
    DBG(payload.toString());
});
```

**前端**（发送事件给 C++ 监听器）：

```javascript
window.__JUCE__.backend.emitEvent("myEventId", { x: 2, y: 6 });
```

### 4.2 C++ 主动推送事件给前端

**C++ 端**：

```cpp
// 在 WebBrowserComponent 可见时发送事件
webComponent.emitEventIfBrowserIsVisible("myEventId", payloadObject);
```

**前端**（接收 C++ 推送）：

```javascript
const removalToken = window.__JUCE__.backend.addEventListener("myEventId", (payload) => {
    console.log(payload); // C++ 发来的数据
});

// 取消订阅
window.__JUCE__.backend.removeEventListener(removalToken);
```

注意：`addEventListener` 返回的是一个 `[eventId, listenerId]` 元组，取消订阅时必须传入这个元组。

### 4.3 内部事件名约定

以 `__juce` 开头的 eventId 是 JUCE 框架保留的内部事件，业务代码不要使用这个前缀。

---

## 五、C++ 端配置详解

### 5.1 WebBrowserComponent::Options 完整配置项

```cpp
juce::WebBrowserComponent::Options options;
options = options
    // ── 后端类型 ──
    .withBackend(WebBrowserComponent::Options::Backend::webview2)  // Windows: 使用 Edge WebView2
    
    // ── 原生集成开关（必须开启，否则 window.__JUCE__ 不会被注入） ──
    .withNativeIntegrationEnabled(true)
    
    // ── Windows WebView2 专用选项 ──
    .withWinWebView2Options(WebBrowserComponent::Options::WinWebView2{}
        .withUserDataFolder(File::getSpecialLocation(File::SpecialLocationType::tempDirectory))
        .withBackgroundColour(Colours::transparentBlack)  // 透明背景
        .withStatusBarDisabled()
        .withBuiltInErrorPageDisabled()
    )
    
    // ── 注册 NativeFunction（JS 可调用的 C++ 函数） ──
    .withNativeFunction("functionName", [](const Array<var>& args, NativeFunctionCompletion complete) {
        // args: JS 传来的参数数组
        // complete: 调用它以返回结果给 JS（可从任意线程调用）
        complete(var("result"));
    })
    
    // ── 注册 EventListener（监听来自 JS 的事件） ──
    .withEventListener("eventId", [](const var& payload) {
        // 处理 JS 发来的事件
    })
    
    // ── 注册 ResourceProvider（为前端提供静态资源） ──
    .withResourceProvider([](const String& url) -> std::optional<WebBrowserComponent::Resource> {
        // 返回资源数据
    })
    
    // ── 关联 Relay 对象（参数同步） ──
    .withOptionsFrom(sliderRelay)
    .withOptionsFrom(toggleRelay)
    .withOptionsFrom(comboBoxRelay)
    
    // ── 注入 JS 脚本（在页面加载前执行） ──
    .withUserScript("console.log('JUCE backend is ready');")
    
    // ── 注入初始化数据 ──
    .withInitialisationData("customKey", "customValue");
```

### 5.2 关键方法

```cpp
// 导航到 URL
webComponent.goToURL("http://localhost:5173/");          // 开发模式
webComponent.goToURL(WebBrowserComponent::getResourceProviderRoot()); // 资源提供者根路径

// 执行 JS（从 C++ 端）
webComponent.evaluateJavascript("console.log('hello');", [](auto result) {
    if (auto* varResult = result.getResult()) {
        // 成功，使用 *varResult
    }
    if (auto* error = result.getError()) {
        // 失败
    }
});

// 推送事件到前端（仅在组件可见时有效）
webComponent.emitEventIfBrowserIsVisible("eventId", payload);

// 静态方法：获取资源提供者的根地址
// Windows/Android: "https://juce.backend/"
// macOS/iOS/Linux: "juce://juce.backend/"
static const String& getResourceProviderRoot();
```

### 5.3 页面生命周期回调

```cpp
// 覆写 WebBrowserComponent 的方法：
virtual bool pageAboutToLoad(const String& newURL);          // 将要导航，返回 false 阻止导航
virtual void pageFinishedLoading(const String& url);          // 页面加载完成
virtual bool pageLoadHadNetworkError(const String& errorInfo); // 网络错误
virtual void windowCloseRequest();                            // JS 请求关闭窗口
virtual void newWindowAttemptingToLoad(const String& newURL); // 尝试打开新窗口
```

---

## 六、ResourceProvider（资源提供者）

ResourceProvider 允许前端通过特定 URL 从 C++ 后端获取资源，无需真实网络请求。

### 6.1 根地址

```javascript
// Windows: "https://juce.backend/"
// macOS/iOS/Linux: "juce://juce.backend/"
const root = window.__JUCE__.initialisationData.__juce__platform[0] === "windows"
  ? "https://juce.backend/"
  : "juce://juce.backend/";
```

或者使用官方封装：
```javascript
import { getBackendResourceAddress } from "juce-framework-frontend";
const url = getBackendResourceAddress("data.json");
```

### 6.2 C++ 端实现

```cpp
options = options.withResourceProvider([](const String& path) -> std::optional<WebBrowserComponent::Resource> {
    // path 格式: "/resourceName"
    if (path == "/index.html") {
        // 返回 HTML
        return WebBrowserComponent::Resource { std::move(htmlBytes), "text/html" };
    }
    if (path.startsWith("/api/")) {
        // 返回 JSON 数据
        String json = "{\"key\": \"value\"}";
        MemoryInputStream stream(json.getCharPointer(), json.getNumBytesAsUTF8(), false);
        return WebBrowserComponent::Resource { streamToVector(stream), "application/json" };
    }
    return std::nullopt; // 未知资源
});
```

### 6.3 前端获取

```javascript
// 直接 fetch
const response = await fetch("https://juce.backend/api/data");
const data = await response.json();

// 或用官方封装
import { getBackendResourceAddress } from "juce-framework-frontend";
const response = await fetch(getBackendResourceAddress("api/data"));
```

### 6.4 支持的 MIME 类型

| 扩展名 | MIME Type |
|--------|-----------|
| .html | text/html |
| .css | text/css |
| .js | text/javascript |
| .json | application/json |
| .png | image/png |
| .jpg / .jpeg | image/jpeg |
| .svg | image/svg+xml |
| .ico | image/vnd.microsoft.icon |
| .woff2 | font/woff2 |

---

## 七、WebControlRelays（参数同步系统）

JUCE 8 提供了预置的参数同步 Relay，用于在 JS UI 控件和 C++ `AudioProcessorParameter` 之间双向同步状态。

### 7.1 三种 Relay

| C++ 类 | JS 构造函数 | 用途 |
|--------|-----------|------|
| `WebSliderRelay(name)` | `getSliderState(name)` | 滑块/旋钮参数 |
| `WebToggleButtonRelay(name)` | `getToggleState(name)` | 开关/复选框参数 |
| `WebComboBoxRelay(name)` | `getComboBoxState(name)` | 下拉列表参数 |

### 7.2 C++ 端使用

```cpp
class MyEditor : public AudioProcessorEditor {
    // 声明 Relay
    WebSliderRelay       cutoffSliderRelay    { "cutoffSlider" };
    WebToggleButtonRelay muteToggleRelay      { "muteToggle" };
    WebComboBoxRelay     filterTypeComboRelay { "filterTypeCombo" };

    // Attachment（连接 Relay 和 AudioParameter）
    WebSliderParameterAttachment       cutoffAttachment;
    WebToggleButtonParameterAttachment muteAttachment;
    WebComboBoxParameterAttachment     filterTypeAttachment;

    SinglePageBrowser webComponent { Options{}
        .withBackend(Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withOptionsFrom(cutoffSliderRelay)     // 关联 Relay
        .withOptionsFrom(muteToggleRelay)
        .withOptionsFrom(filterTypeComboRelay)
    };

public:
    MyEditor(MyAudioProcessor& p)
        : cutoffAttachment(*p.state.getParameter("cutoff"), cutoffSliderRelay, p.state.undoManager),
          muteAttachment(*p.state.getParameter("mute"), muteToggleRelay, p.state.undoManager),
          filterTypeAttachment(*p.state.getParameter("filterType"), filterTypeComboRelay, p.state.undoManager)
    {}
};
```

### 7.3 JS 端使用

```javascript
import { getSliderState, getToggleState, getComboBoxState } from "juce-framework-frontend";

// Slider
const slider = getSliderState("cutoffSlider");
slider.setNormalisedValue(0.5);                       // 设置归一化值 [0, 1]
const val = slider.getScaledValue();                   // 获取实际缩放值
slider.sliderDragStarted();                            // 开始拖拽
slider.sliderDragEnded();                              // 结束拖拽
slider.valueChangedEvent.addListener(() => { ... });   // 监听值变化
slider.propertiesChangedEvent.addListener(() => { ... }); // 监听属性变化

// Toggle
const toggle = getToggleState("muteToggle");
toggle.setValue(true);
toggle.getValue();
toggle.valueChangedEvent.addListener(() => { ... });

// ComboBox
const combo = getComboBoxState("filterTypeCombo");
combo.setChoiceIndex(0);                               // 选择第 0 项
const idx = combo.getChoiceIndex();                    // 当前选中索引
combo.properties.choices;                              // ["Low-pass", "High-pass", "Band-pass"]
```

### 7.4 Relay 通信协议

每个 Relay 有一个唯一的事件 ID（`__juce__slider` + name / `__juce__toggle` + name / `__juce__comboBox` + name），其事件格式如下：

```json
// Slider: 值变化
{ "eventType": "valueChanged", "value": 0.5 }

// Slider: 拖拽开始/结束
{ "eventType": "sliderDragStarted" }
{ "eventType": "sliderDragEnded" }

// Slider: 请求初始值同步
{ "eventType": "requestInitialUpdate" }

// Toggle/ComboBox: 值变化
{ "eventType": "valueChanged", "value": true }
{ "eventType": "valueChanged", "value": 0.0 }

// Toggle/ComboBox: 请求初始值同步
{ "eventType": "requestInitialUpdate" }

// 属性变更 (由 C++ → JS)
{ "eventType": "propertiesChanged", "start": 0, "end": 1, "skew": 1, ... }
```

---

## 八、ControlParameterIndex（DAW 宿主参数索引）

用于在 DAW 宿主中实现 `AudioProcessorEditor::getControlParameterIndex()`，让宿主知道鼠标正悬停在哪个参数上。

### 8.1 C++ 端

```cpp
class MyEditor : public AudioProcessorEditor {
    WebControlParameterIndexReceiver controlParameterIndexReceiver;

    int getControlParameterIndex(Component&) override {
        return controlParameterIndexReceiver.getControlParameterIndex();
    }

    WebBrowserComponent webComponent { Options{}
        .withOptionsFrom(controlParameterIndexReceiver)  // 关联
    };
};
```

### 8.2 JS 端

```javascript
import { ControlParameterIndexUpdater } from "juce-framework-frontend";

const updater = new ControlParameterIndexUpdater("controlparameterindex");
document.addEventListener("mousemove", (event) => {
    updater.handleMouseMove(event);
});

// 在 DOM 元素上标记参数索引
<div controlparameterindex="0">   <!-- 参数索引 0 -->
<div controlparameterindex="1">   <!-- 参数索引 1 -->
```

---

## 九、本项目桥接层架构

### 9.1 文件结构

```
UI/
├── bridgeDefs.json              # 桥接函数定义（函数名注册表）
├── generateBridge.cjs           # 代码生成脚本
└── src/bridge/
    ├── bridge.generated.ts       # 自动生成: BRIDGE_KEYS 常量和类型
    ├── juceMacro.ts              # JUCE 内部事件 key 常量
    └── initBridge.ts             # 自定义 Promise 桥接层（带错误处理）
```

### 9.2 添加新的桥接函数

**步骤 1** — 在 `bridgeDefs.json` 中注册：

```json
{
  "inputFiles": "inputFiles",
  "toggleMyLike": "toggleMyLike",
  "newFunction": "newFunction"
}
```

**步骤 2** — 运行生成脚本（已挂载到 CMake build 流程）：

```
node UI/generateBridge.cjs
```

这会自动更新：
- `UI/src/bridge/bridge.generated.ts`（TS 端常量）
- `Utils/BridgeNames.h`（C++ 端常量，生成为 `BridgeKeys::xxx`）

**步骤 3** — 在 `MainComponent.cpp` 中注册 C++ 回调：

```cpp
options = options.withNativeFunction(BridgeKeys::newFunction,
    [](const Array<var>& args, NativeFunctionCompletion complete) {
        // 处理逻辑...
        // 成功: complete(var());
        // 失败: 创建 DynamicObject 带 "error" 属性
    }
);
```

**步骤 4** — 在前端调用：

```typescript
import { callJuceFunc } from "./bridge/initBridge.ts"
import { BRIDGE_KEYS } from "./bridge/bridge.generated.ts"

const result = await callJuceFunc(BRIDGE_KEYS.newFunction, arg1, arg2)
```

### 9.3 前端调用 `callJuceFunc` vs 直接使用 `backend.emitEvent`

```typescript
// ✅ 推荐：使用 callJuceFunc (带错误处理和 Promise 封装)
const result = await callJuceFunc(BRIDGE_KEYS.inputFiles)

// ❌ 不推荐：直接操作底层 API
window.__JUCE__.backend.emitEvent("__juce__invoke", {
  name: "inputFiles",
  params: [],
  resultId: 123
})
```

### 9.4 C++ 端 `complete` 回调规则

1. **必须调用 `complete()`**：即使不需要返回数据，也要调用 `complete(var())`
2. **可从任意线程调用**：`NativeFunctionCompletion` 内部会自动切回 Message Thread
3. **错误处理**：返回带有 `"error"` 属性的对象会被前端自动识别为 rejection：
   ```cpp
   auto error = new DynamicObject();
   error->setProperty("error", "错误消息字符串");
   complete(var(error));
   ```

---

## 十、常见模式

### 10.1 前端监听 C++ 推送

```typescript
// 订阅
const token = window.__JUCE__.backend.addEventListener("songChanged", (payload) => {
    console.log("歌曲已切换:", payload);
});

// 组件销毁时取消订阅
window.__JUCE__.backend.removeEventListener(token);
```

### 10.2 C++ 定时推送数据给前端

```cpp
// 参考 WebViewPluginDemo.h 中的 timerCallback 实现
void timerCallback() override {
    Array<var> data;
    // ... 填充数据 ...
    webComponent.emitEventIfBrowserIsVisible("dataUpdate", data);
}
```

### 10.3 阻止页面导航（单页应用）

```cpp
struct SinglePageBrowser : WebBrowserComponent {
    using WebBrowserComponent::WebBrowserComponent;
    bool pageAboutToLoad(const String& newURL) override {
        // 只允许开发服务器和资源提供者地址
        return newURL == "http://localhost:5173/" 
            || newURL == getResourceProviderRoot();
    }
};
```

### 10.4 从 C++ 执行 JS

```cpp
webComponent.evaluateJavascript("document.title", [](auto result) {
    if (auto* value = result.getResult())
        DBG("页面标题: " << value->toString());
});
```

---

## API查询路径

如果该文档没有你需要的内容，可以直接前往 JUCE 源码：

| 文件 | 内容 |
|------|------|
| `build/debug/_deps/juce-src/examples/Plugins/WebViewPluginDemo.h` | 后端 WebView 完整实现示例 |
| `build/debug/_deps/juce-src/examples/Plugins/WebViewPluginDemoGUI/src/App.js` | 前端 React WebView 实现示例 |
| `build/debug/_deps/juce-src/modules/juce_gui_extra/native/javascript/index.js` | JUCE 官方 JS 前端库（所有 API） |
| `build/debug/_deps/juce-src/modules/juce_gui_extra/native/javascript/check_native_interop.js` | 底层注入脚本（`window.__JUCE__` 初始化） |
| `build/debug/_deps/juce-src/modules/juce_gui_extra/misc/juce_WebBrowserComponent.h` | WebBrowserComponent 头文件（Options API） |
| `build/debug/_deps/juce-src/modules/juce_gui_extra/misc/juce_WebBrowserComponent.cpp` | WebBrowserComponent 实现（底层通信机制） |
| `build/debug/_deps/juce-src/modules/juce_gui_extra/misc/juce_WebControlRelays.h` | WebSliderRelay / WebToggleButtonRelay / WebComboBoxRelay 定义 |
| `build/debug/_deps/juce-src/modules/juce_gui_extra/misc/juce_WebControlRelays.cpp` | Relay 实现（事件协议细节） |
| `build/debug/_deps/juce-src/modules/juce_gui_extra/misc/juce_WebControlParameterIndexReceiver.h` | DAW 宿主参数跟踪 |
