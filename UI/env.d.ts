/// <reference types="vite/client" />
//env.d.ts 是 TypeScript 的“全局声明文件”，它的核心作用是“告诉 TypeScript 编译器，在运行时环境里本来就有这些东西，你不要给我报红

interface Window {//在浏览器里打开任何一个网页，JS 引擎都会自动创建一个 window 对象。它代表当前的浏览器窗口/标签页。
  __JUCE__: {
    initialisationData: {
      __juce__platform: string[];
      __juce__functions: string[];
      __juce__registeredGlobalEventIds: string[];
      __juce__sliders: string[];
      __juce__toggles: string[];
    };
    backend: {
      /**
       * JUCE 原生事件通信
       * - emitEvent("__juce__invoke", {name, params, resultId}) → JS → C++ 调用原生函数
       * - emitEvent 不返回 Promise，C++ 的结果通过 "__juce__complete" 事件异步回传
       */
      emitEvent(eventName: string, payload?: unknown): void;//问号代表这个参数是可选的
      addEventListener(eventName: string, callback: (payload: any) => void): void;
      removeEventListener(eventName: string, callback: (payload: any) => void): void;
      emitByBackend(eventName: string, payload: unknown): void;
    };
    /** C++ → JS 单向推送消息 */
    postMessage(object: unknown): void;
  };
}
// TypeScript里面的{...}
// 只存在于编译阶段(写代码时)。
// 编译成JS后，完全消失，不产生任何代码。
// 这是匿名的(没给它起名字，直接内嵌)。
// 只能描述类型(有哪些函数、参数类型是什么)，不能存值，不能有实现。

// env.d.ts（你现在的写法）：这个文件没有任何 import 或 export 语句。
// TypeScript 把它视为一个 “全局脚本（Global Script）”。在
// 全局脚本里，你写的 interface Window 会直接与 TypeScript 内置的 Window 接口合并，所以全项目生效。

// main.ts（你想放进去的地方）：这个文件的顶部一定有 import { createApp } from 'vue'。
// 只要文件里有任何一个 import 或 export，TypeScript 就把这个文件视为一个“模块（Module）”。
// 在模块内部，你写的 interface Window 不再代表“全局的 Window”，而是代表“模块内部一个叫 Window 的局部接口”。
// 它不会与浏览器的 Window 合并，所以你写 window.__JUCE__ 依然报红。
