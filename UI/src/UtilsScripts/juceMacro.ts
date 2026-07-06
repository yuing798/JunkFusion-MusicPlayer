
export const JUCE_KEYS = {
  __juce__invoke: '__juce__invoke' as const,//前端调用 C++ 注册的 Native 异步函数时，底层 JS 框架用来将函数名、参数打包并分发给 C++ 的核心事件标识符
  __juce__complete:'__juce__complete' as const,
} as const;