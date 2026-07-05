/// <reference types="vite/client" />

declare global {
  interface Window {
    __JUCE__: {
      backend: {
        [key: string]: (...args: any[]) => Promise<any>;
      };
    };
  }
}

export {}; // 确保这是模块