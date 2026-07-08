import type { BridgeFunctionName } from "./bridge.generated.ts"
import { JUCE_KEYS } from "./juceMacro.ts"

// ════════════════════════════════════════════════════════════════════
// 环境检测：只在 JUCE WebView2 内部才存在 window.__JUCE__，
// 普通浏览器（npm run dev）中没有这个对象，必须跳过桥接初始化，
// 否则访问 undefined.backend 会导致 JS 崩溃、页面一片空白。
// ════════════════════════════════════════════════════════════════════
const isInsideJUCE = typeof window !== 'undefined' && window.__JUCE__ !== undefined

let lastPromiseId = 0

// 因为 lastPromiseId 不需要触发页面重新渲染（不需要响应式），且必须被重新赋值（++），所以用 let 是最佳选择。

// 展开讲三个层面：

// 为什么不能用 const：你在 callNativeFunction 里写了 const promiseId = lastPromiseId++，
// 这是一个重新赋值操作（lastPromiseId = lastPromiseId + 1）。
// const 定义的变量不能被重新赋值，所以用 const 会直接报错 Assignment to constant variable。必须用 let。

// 为什么不用 ref（Vue 响应式）：
// ref 的作用：让数据变化时，用到它的 Vue 模板/计算属性/侦听器能自动更新。
// 你的 lastPromiseId 只用在哪里？ 只用在 callNativeFunction 内部做自增计数，
// 然后拼接到 promiseId 里传给 C++。页面上没有任何地方显示这个 ID，也没有任何计算属性依赖它。
// 用 ref 会引入不必要的性能开销（虽然很小，但属于”杀鸡用牛刀”），而且每次取值还得写 lastPromiseId.value++，平添代码噪音。

const pendingPromises = new Map<
  number,
  {
    resolve: (v: unknown) => void
    reject: (e: unknown) => void
  }
>()
//new 是 JavaScript 的实例化运算符。它的作用是”调用一个类的构造函数，在内存中造出一个实实在在的对象”。
// 在这里，new Map() 就是在内存中造出一个全新的 Map 数据集合（就像一个刚从文具店买回来的空白账本）。
//但是在js中不需要手动delete,因为js有垃圾回收机制

// Map 是 JavaScript 内置的键值对（Key-Value）集合。你可以把它想象成一个更强大的对象（Object）。
// 相比于普通对象 {}，Map 有两大优势：
// 键（Key）可以是任何类型（数字、字符串、对象，甚至函数），而普通对象的键只能被强制转成字符串。
// 有顺序，而且 size 属性可以随时告诉你里面存了多少项，非常适合频繁增删改查。

// < > 是 TypeScript 的泛型（Generics） 语法。它只存在于编译阶段，用来给 TypeScript 的智能提示和类型检查提供”蓝图”。
// 这里 Map<number, { resolve: ..., reject: ... }> 就是在告诉 TS：
// “这个 Map 的键（Key）必须是 number 类型，值（Value）必须是一个包含 resolve 和 reject 两个函数的对象。”

// new Map<...>()，末尾的 () 是调用构造函数。
// 即使你没有传任何参数进去（空括号 ()），这个括号也必须写，
// 因为只有加上括号，JS 引擎才会去执行 Map 类的构造函数，生成一个实实在在的空 Map 实例。

/**
 * JUCE 原生函数桥接 — 与 juce-framework-frontend 的 getNativeFunction 等价
 *
 * 调用方式来自 JUCE 源码 modules/juce_gui_extra/native/javascript/index.js：
 *   emitEvent(“__juce__invoke”, {name, params, resultId})
 * 然后监听 “__juce__complete” 拿到返回值并 resolve Promise
 */

// 注册 __juce__complete 监听器（只需一次）
// 注意：只在 JUCE 环境中注册，普通浏览器跳过此步
if (isInsideJUCE) {
  window.__JUCE__.backend.addEventListener(
    JUCE_KEYS.__juce__complete,
    (payload: { promiseId: number; result: unknown ; error:string}) => {
      const { promiseId, result ,error} = payload
      const pending = pendingPromises.get(promiseId)//根据 ID 查找待办事项
      if (pending) {
        if(error){
          pending.reject(new Error(error))
        }else{
          pending.resolve(result)//唤醒 Promise（成功）
        }
        pendingPromises.delete(promiseId)//清理”待办事项”（防止内存泄漏）
        //这一步极其重要！如果不删除，这个条目会永远留在 pendingPromises 里。
        // 随着用户操作越来越多，Map 会无限膨胀，最终导致前端内存泄漏（页面卡顿崩溃）
      }
    },
  )
}
// 这段代码里的 addEventListener 并不是在“定义函数”，而是在“调用函数”。
// 它的“函数体”不在你的 JS/TS 代码里，而是存在于 C++（JUCE 原生代码）中。

// .get() 是 JavaScript 中 Map 对象的内置方法，
// 作用是根据键（Key）去 Map 里查找并返回对应的值（Value）。pendingPromises 里存的不是属性名
// ，而是键值对（Key-Value Pair）。它的键是数字（number），值是你存进去的 { resolve, reject } 对象。

// 这段代码是你整个 Vue ↔ JUCE 异步桥接的“接收器”。它的完整作用是：
// 持续监听 C++ 端发来的 __juce__complete 事件，当收到结果时，根据携带的 promiseId 找到之前挂起的 Promise，并“唤醒”它（传递结果或错误）。

// window.__JUCE__.backend：由 C++（JUCE）在 WebView 启动时注入到浏览器中的全局对象。它是 JS 与 C++ 通信的“总入口”。
// '__juce__complete'：事件名称。这是一个协议约定，你和 C++ 端必须都用这个字符串。C++ 端完成任务后，会主动触发（emit）这个名称的事件。

export function callNativeFunction(name: BridgeFunctionName, ...args: unknown[]): Promise<unknown> {
  const promiseId = lastPromiseId++
  const promise = new Promise<unknown>((resolve, reject) => {
    pendingPromises.set(promiseId, { resolve, reject })
  })

  window.__JUCE__.backend.emitEvent(JUCE_KEYS.__juce__invoke, {
    name,
    params: args,
    resultId: promiseId,
  })

  return promise
}