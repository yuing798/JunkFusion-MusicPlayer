/// <reference types="vite/client" />

declare global {
  interface Window {
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
        emitEvent(eventName: string, payload?: unknown): void;
        addEventListener(eventName: string, callback: (payload: any) => void): void;
        removeEventListener(eventName: string, callback: (payload: any) => void): void;
        emitByBackend(eventName: string, payload: unknown): void;
      };
      /** C++ → JS 单向推送消息 */
      postMessage(object: unknown): void;
    };
  }
}

export {};
