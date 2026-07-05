# JUCE WebView2 桥接文档

本文档说明 C++（JUCE）与 JS（Vue）之间如何通过 WebView2 进行双向通信。

---

## 目录

- [整体架构](#整体架构)
- [通信协议](#通信协议)
- [注册新函数：三步流程](#注册新函数三步流程)
- [JS 端调用方式](#js-端调用方式)
- [callNativeFunction 源码解释](#callnativefunction-源码解释)
- [数据类型映射](#数据类型映射)
- [环境声明](#环境声明)

---

## 整体架构

```
┌─────────────────────────────────────────────────────┐
│                    C++ (JUCE)                        │
│                                                     │
│  .withNativeFunction("inputFiles", callback)         │
│       │                                             │
│       │  JUCE 内部监听 "__juce__invoke" 事件          │
│       │  根据 payload.name 路由到对应的回调函数         │
│       │  回调执行完成后，通过 complete() 发回结果       │
│       │                                             │
└───────┼─────────────────────────────────────────────┘
        │  WebView2 IPC
┌───────┼─────────────────────────────────────────────┐
│  window.__JUCE__.backend                            │
│       │                                             │
│  emitEvent("__juce__invoke", {name, params, resultId})│
│       │                                             │
│  addEventListener("__juce__complete", callback)      │
│                                                     │
│                    Vue / JS                          │
└─────────────────────────────────────────────────────┘
```

**关键前提**：使用 `NEEDS_WEBVIEW2 TRUE` 时，JUCE 会自动注入 `juce_gui_extra/native/javascript/index.js` 到每个页面中。这个脚本创建了 `window.__JUCE__` 全局对象，提供了 `backend.emitEvent` 和 `backend.addEventListener` 两个核心方法。

---

## 通信协议

JUCE 8 的原生函数调用依赖两个内部事件：

### 1. JS → C++：调用原生函数

```
JS: backend.emitEvent("__juce__invoke", {
    name:     "inputFiles",       // 函数名，对应 .withNativeFunction() 注册的名字
    params:   [],                 // 传给 C++ 的参数数组
    resultId: 0,                  // Promise ID，用于匹配返回值
})
```

JUCE 内部接收到 `__juce__invoke` 事件后，根据 `name` 查找已注册的原生函数并调用它。

### 2. C++ → JS：返回结果

C++ 回调中调用 `complete(value)` 后，JUCE 自动向 JS 发送：

```
C++ → JS: backend.emitEvent("__juce__complete", {
    promiseId: 0,                 // 匹配 JS 端的 Promise ID
    result:    [...]              // complete() 传入的值
})
```

JS 端监听 `__juce__complete` 取出 `result`，resolve 对应的 `Promise`。

### 视觉流程

```
JS 端                              C++ 端
  │                                  │
  │  callNativeFunction("inputFiles")│
  │  ├─ new Promise → promiseId: 0   │
  │  ├─ emitEvent("__juce__invoke",  │
  │  │    {name, params, resultId})──┼──→  JUCE 路由 → 调用
  │  │                              │     .withNativeFunction 回调
  │  │                              │     ├─ 执行业务逻辑
  │  │                              │     └─ complete(result)
  │  │                              │           │
  │  │  emitEvent("__juce__complete", │←─────────┘
  │  │    {promiseId, result})       │
  │  │←─────────────────────────────│
  │  ├─ resolve(result)              │
  │  └─ await → 拿到返回值           │
  ▼                                  ▼
```

---

## 注册新函数：三步流程

假设要添加一个 `getSongList` 函数，JS 调用它获取歌曲列表。

### 步骤 1：在 `bridgeDefs.json` 中定义

```json
{
  "inputFiles": "inputFiles",
  "getSongList": "getSongList"
}
```

然后运行 `node generateBridge.cjs` 重新生成桥接文件。

这会自动更新：
- `UI/src/bridge.generated.ts` → JS 端常量
- `Utils/BridgeNames.h` → C++ 端常量

### 步骤 2：在 C++ 注册原生函数

```cpp
// MainComponent.cpp → MainComponent::MainComponent() 中

options = options
    .withNativeFunction(BridgeKeys::inputFiles, callback)   // 已有的

    // 新增
    .withNativeFunction(BridgeKeys::getSongList,
        [](const juce::Array<juce::var>& args,
           juce::WebBrowserComponent::NativeFunctionCompletion complete)
        {
            // 业务逻辑
            std::vector<SongInfo> songs = ...;

            // 转成 juce::var 数组返回给 JS
            juce::Array<juce::var> results;
            for (auto& song : songs)
                results.add(SongInfo::toVar(song));

            complete(juce::var(results));
        }
    );
```

**重要**：如果要调用 UI 相关的 API（如 `FileChooser`、`getParentComponent`），必须在消息线程上执行：

```cpp
.withNativeFunction(BridgeKeys::inputFiles,
    [this](const juce::Array<juce::var>& args,
           juce::WebBrowserComponent::NativeFunctionCompletion complete)
    {
        // WebView2 的回调线程不是消息线程
        juce::MessageManager::callAsync([this, complete, args](){
            // 现在在消息线程上，可以安全操作 UI
            getMultiMediaFileChoose(...);
        });
    }
);
```

### 步骤 3：在 JS 中调用

```ts
// 任意 .vue 文件中
import { BRIDGE_KEYS } from '@/bridge.generated'

// 使用 callNativeFunction
const songs = await callNativeFunction(BRIDGE_KEYS.getSongList)
console.log(songs)  // [{filePath: "...", title: "..."}, ...]
```

---

## JS 端调用方式

目前 `callNativeFunction` 写在 `AllMusic.vue` 中。当其他组件也需要它时，建议把它抽到一个单独的 `utils/juceBridge.ts` 模块中，然后各处 import。

### 临时用法（复制到需要的组件中）

```ts
// ── 桥接基础设施（每个需要调 C++ 的组件都要复制这一段）──

let lastPromiseId = 0
const pendingPromises = new Map<
  number,
  { resolve: (v: unknown) => void; reject: (e: unknown) => void }
>()

window.__JUCE__.backend.addEventListener(
  '__juce__complete',
  (payload: { promiseId: number; result: unknown }) => {
    const { promiseId, result } = payload
    const pending = pendingPromises.get(promiseId)
    if (pending) {
      pending.resolve(result)
      pendingPromises.delete(promiseId)
    }
  },
)

function callNativeFunction(name: string, ...args: unknown[]): Promise<unknown> {
  const promiseId = lastPromiseId++
  const promise = new Promise<unknown>((resolve, reject) => {
    pendingPromises.set(promiseId, { resolve, reject })
  })

  window.__JUCE__.backend.emitEvent('__juce__invoke', {
    name,
    params: args,
    resultId: promiseId,
  })

  return promise
}

// ── 业务调用 ──
const result = await callNativeFunction(BRIDGE_KEYS.getSongList, page, ascending, sortMode)
```

### 推荐：抽取为公共模块

```ts
// UI/src/utils/juceBridge.ts
let lastPromiseId = 0
const pendingPromises = new Map<...>()

window.__JUCE__.backend.addEventListener('__juce__complete', ...)

export function callNativeFunction(name: string, ...args: unknown[]): Promise<unknown> {
  // ...同上
}
```

然后在组件中：

```ts
import { callNativeFunction } from '@/utils/juceBridge'
import { BRIDGE_KEYS } from '@/bridge.generated'

const result = await callNativeFunction(BRIDGE_KEYS.getSongList)
```

---

## callNativeFunction 源码解释

以下代码位于 `AllMusic.vue` 第 60-99 行：

```ts
// 自增计数器，生成唯一的 promiseId
let lastPromiseId = 0

// 用 Map 存储所有"发出去了但还没收到回复"的 Promise
const pendingPromises = new Map<
  number,
  { resolve: (v: unknown) => void; reject: (e: unknown) => void }
>()

// 全局只注册一次：监听来自 C++ 的返回值
window.__JUCE__.backend.addEventListener(
  '__juce__complete',
  (payload: { promiseId: number; result: unknown }) => {
    const pending = pendingPromises.get(payload.promiseId)
    if (pending) {
      pending.resolve(payload.result)     // 让 await 拿到值
      pendingPromises.delete(payload.promiseId)  // 清理
    }
  },
)

function callNativeFunction(name: string, ...args: unknown[]): Promise<unknown> {
  const promiseId = lastPromiseId++       // 分配 ID
  const promise = new Promise<unknown>((resolve, reject) => {
    pendingPromises.set(promiseId, { resolve, reject })  // 存入 Map
  })

  window.__JUCE__.backend.emitEvent('__juce__invoke', {
    name,                                 // C++ 端注册的函数名
    params: args,                         // 传给 C++ 的参数
    resultId: promiseId,                  // 用于匹配返回值
  })

  return promise                          // 调用方可以 await
}
```

### 为什么不用 juce-framework-frontend？

官方 npm 包 `juce-framework-frontend` 被设计为 JUCE 插件项目的 `src/` 目录内的本地依赖，需要在 C++ 中通过 `withResourceProvider` 提供。我们的架构是 Vite 独立开发服务器（`withNativeIntegrationEnabled(true)`），所以直接使用官方的底层 API 更简单。

---

## 数据类型映射

### JS → C++（参数传递）

| JS 类型 | C++ `juce::var` 取值方式 |
|---|---|
| `number` | `(int) var` 或 `(double) var` |
| `boolean` | `(bool) var` |
| `string` | `var.toString()` |
| `Array` | `juce::Array<juce::var>`，通过 `var[index]` 访问 |
| `undefined` | `var.isVoid() == true` |

### C++ → JS（complete 返回值）

`juce::var` 支持的类型会自动映射到 JS，不需要做任何额外工作：

| C++ 端 `juce::var` | JS 端 `await` 拿到的 |
|---|---|
| `juce::var("hello")` | `"hello"` |
| `juce::var(42)` | `42` |
| `juce::var(true)` | `true` |
| `juce::var()` | `undefined` |
| `juce::var(Array<var>)` | JS `Array` |
| `juce::var(DynamicObject*)` | JS `Object` |
| `SongInfo::toVar(song)` | JS `{filePath: "...", title: "..."}` |

**示例**：`SongInfo::toVar` 返回的 `juce::var(DynamicObject)` 会自动变成：

```js
{
  filePath: "C:/music/xxx.mp3",
  fileSize: 10485760,
  title: "曲名",
  artist: "歌手",
  duration: 243.5,
  // ... 所有 SongInfo 字段
}
```

---

## C++ 完整注册模板

```cpp
#include "Utils/BridgeNames.h"

// 在 MainComponent::MainComponent() 中
juce::WebBrowserComponent::Options options;
options = options
    .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
    .withNativeIntegrationEnabled(true)

    // ─── 注册所有原生函数 ───

    .withNativeFunction(BridgeKeys::inputFiles,
        [this](const juce::Array<juce::var>& args,
               juce::WebBrowserComponent::NativeFunctionCompletion complete)
        {
            // 需要 UI 操作 → 必须切到消息线程
            juce::MessageManager::callAsync([this, complete, args](){
                // ... 业务逻辑 ...
                complete(juce::var(results));
            });
        }
    )

    .withNativeFunction(BridgeKeys::getSongList,
        [](const juce::Array<juce::var>& args,
           juce::WebBrowserComponent::NativeFunctionCompletion complete)
        {
            // 纯数据操作（不操作 UI）→ 不需要切线程
            int page      = args.size() > 0 ? (int)args[0] : 0;
            bool ascending = args.size() > 1 ? (bool)args[1] : true;

            auto songs = SongsManage::getInstance().getSongPage(...);

            juce::Array<juce::var> results;
            for (auto& song : songs)
                results.add(SongInfo::toVar(song));
            complete(juce::var(results));
        }
    );

web = std::make_unique<juce::WebBrowserComponent>(options);
```

---

## 环境声明

`UI/env.d.ts` 定义了 `window.__JUCE__` 的类型，让 TypeScript 不报 `ts-plugin(2722)` 错误。当发现新的事件或方法时，在此文件中补充类型声明。
