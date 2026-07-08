/// <reference types="vite/client" />
//env.d.ts 是 TypeScript 的“全局声明文件”，它的核心作用是“告诉 TypeScript 编译器，在运行时环境里本来就有这些东西，你不要给我报红

interface Window {//在浏览器里打开任何一个网页，JS 引擎都会自动创建一个 window 对象。它代表当前的浏览器窗口/标签页。
  __JUCE__: {
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