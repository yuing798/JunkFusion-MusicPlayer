import type { BridgeFunctionName } from "./bridge.generated.ts"
import { JUCE_KEYS } from "./juceMacro.ts"

// ════════════════════════════════════════════════════════════════════
// 环境检测：只在 JUCE WebView2 内部才存在 window.__JUCE__，
// 普通浏览器（npm run dev）中没有这个对象，必须跳过桥接初始化，
// 否则访问 undefined.backend 会导致 JS 崩溃、页面一片空白。
// ════════════════════════════════════════════════════════════════════

// T0: callJuceFunc 被调用
//     → new Promise 创建盒子（状态 pending）
//     → 把 resolve/reject 存进 Map（钥匙是 promiseId）
//     → 立即返回这个盒子（promise）给业务代码
//     → 业务代码执行 await callJuceFunc()，被挂起（等待）

// T1: emitEvent 把 { name, params, resultId } 发给 C++
//     → C++ 开始处理（耗时操作）

// T2（500ms 后）: C++ 处理完，触发 __juce__complete
//     → 监听器收到 payload（含 promiseId 和 result）
//     → 从 Map 里取出对应的 resolve 函数
//     → 执行 resolve(result) → 盒子被填满 → await 恢复执行

const isInsideJUCE = typeof window !== 'undefined' && window.__JUCE__ !== undefined

let lastPromiseId = 0//最后一次调用juce函数的id号

const pendingPromises = new Map<
  number,
  {
    resolve: (v: unknown) => void;
    reject: (e: unknown) => void;
  }
>();

// 注册 __juce__complete 监听器（只需一次）
// 注意：只在 JUCE 环境中注册，普通浏览器跳过此步，否则直接用浏览器打开会一片空白，不方便调试UI
if (isInsideJUCE) {
  window.__JUCE__.backend.addEventListener(
    JUCE_KEYS.__juce__complete,
    (payload: { promiseId: number; result: unknown}) => {
      const { promiseId, result} = payload
      const pending = pendingPromises.get(promiseId)//根据 ID 查找待办事项
      if (pending) {
        if(result&&typeof result === 'object'&&(result as any).error){
          pending.reject(new Error((result as any).error))
        }else{
          pending.resolve(result)
        }
        pendingPromises.delete(promiseId)//清理”待办事项”（防止内存泄漏）
      }
    },
  )
}

export function callJuceFunc(name: BridgeFunctionName, ...args: unknown[]): Promise<unknown> {
  const promiseId = lastPromiseId++
  const promise = new Promise<unknown>((resolve,reject) => {
    pendingPromises.set(promiseId, {resolve,reject})
  })

  window.__JUCE__.backend.emitEvent(JUCE_KEYS.__juce__invoke, {
    name,
    params: args,
    resultId: promiseId,
  })

  return promise
}