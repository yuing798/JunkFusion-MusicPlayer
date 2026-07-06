<script setup lang="ts">
import { ref, nextTick, onUnmounted, type CSSProperties } from 'vue'

// ════════════════════════════════════════════════════════════════
// Tooltip — 鼠标悬停提示窗
//
// 使用方式：
//   <Tooltip text="这是提示文本">
//     <button>悬停我</button>
//   </Tooltip>
//
//   <Tooltip :text="dynamicText">
//     <SomeComponent />
//   </Tooltip>
//
// 特性：
//   - 鼠标悬停 300ms 后出现
//   - 位置跟随鼠标，自动避让屏幕边界
//   - 通过 Teleport 渲染到 body，不受应用窗口裁剪
// ════════════════════════════════════════════════════════════════

// ── Props ──
//“声明子组件可以接收哪些外部数据”的编译器宏
const props = defineProps<{
  /** 外部输入文本的接口 — 父组件通过此 prop 设置提示内容 */
  text: string
}>()

// 父 → 子（数据下行）	defineProps	像是 C++ 函数的输入参数。
// 子 → 父（事件上行）	defineEmits	像是 C++ 函数的返回值或回调通知。

// ── 可见性与位置状态 ──
const visible = ref(false)
const popupX = ref(0)
const popupY = ref(0)

// ── 鼠标跟踪 ──
let mouseX = 0
let mouseY = 0
let showTimer: ReturnType<typeof setTimeout> | null = null

// ReturnType<...>：这是 TypeScript 内置的工具类型。它接受一个函数类型，提取出该函数的返回值类型。

// tooltip 弹出层的 DOM 引用，用于测量尺寸
const popupRef = ref<HTMLElement | null>(null)

// ── 定位常量 ──
const GAP = 12 // 提示窗与鼠标指针之间的间距（px）
const popupTime = 300 //鼠标悬浮在组件上直到出现的时间

/**
 * 根据当前鼠标位置和 tooltip 尺寸计算最终坐标，
 * 确保不超出屏幕可见区域。
 *
 * 默认：提示窗出现在鼠标右下方（偏移 GAP px）
 * 如果右侧溢出 → 翻转到鼠标左侧
 * 如果下方溢出 → 翻转到鼠标上方
 */
function computePosition(): void {
  if (!popupRef.value) return

  const rect = popupRef.value.getBoundingClientRect()
  const { innerWidth, innerHeight } = window

  let x = mouseX + GAP
  let y = mouseY + GAP

  // 右侧溢出 → 翻转到左侧
  if (x + rect.width > innerWidth) {
    x = mouseX - rect.width - GAP
  }
  // 下方溢出 → 翻转到上方
  if (y + rect.height > innerHeight) {
    y = mouseY - rect.height - GAP
  }
  // 兜底：不允许坐标为负（贴边显示）
  if (x < 0) x = GAP
  if (y < 0) y = GAP

  popupX.value = x
  popupY.value = y
}

/**
 * 显示 tooltip 并异步计算位置。
 * 用 requestAnimationFrame 确保 DOM 已挂载且浏览器完成布局后再测量。
 */
function show(): void {
  visible.value = true
  // nextTick 等待 Vue 完成 DOM 更新 + 组件挂载到 body
  // requestAnimationFrame 等待浏览器完成布局计算
  void nextTick(() => {
    requestAnimationFrame(() => {
      computePosition()
    })
  })
}

function hide(): void {
  cancelTimer()
  visible.value = false
}

function cancelTimer(): void {
  if (showTimer !== null) {
    clearTimeout(showTimer)
    showTimer = null
  }
}

// ── 事件处理 ──

function onMouseEnter(e: MouseEvent): void {
  mouseX = e.clientX
  mouseY = e.clientY
  cancelTimer()
  showTimer = setTimeout(show, 300)
}

function onMouseMove(e: MouseEvent): void {
  mouseX = e.clientX
  mouseY = e.clientY
  if (visible.value) {
    // 提示窗已经可见，实时跟随鼠标
    // 不在这里做防抖，因为 getBoundingClientRect 是同步的且代价很低
    requestAnimationFrame(() => {
      computePosition()
    })
  }
}

function onMouseLeave(): void {
  hide()
}

// 组件卸载时清理定时器，防止内存泄漏
onUnmounted(() => {
  cancelTimer()
})
</script>

<template>
  <!--
    宿主容器：包裹触发元素，捕获鼠标事件。
    display: contents 的作用是让这个 div 不参与布局计算，
    这样它不会干扰父组件的 flex/grid 布局，
    同时 children 的鼠标事件能正常冒泡到这一层。

    在 WebView2（Chromium）中，display: contents 元素的
    事件监听器可以正常工作。
  -->
  <div
    class="tooltip-host"
    @mouseenter="onMouseEnter"
    @mousemove="onMouseMove"
    @mouseleave="onMouseLeave"
  >
    <slot />
  </div>

  <!--
    Teleport to body：
    将 tooltip 弹出层渲染到 <body> 下，
    使其脱离应用窗口的 DOM 树，使用 position: fixed
    相对于整个屏幕视口定位，不会被子窗口裁剪（overflow: hidden 等）。
  -->
  <Teleport to="body">
    <div
      v-if="visible"
      ref="popupRef"
      class="tooltip-popup"
      :style="{
        left: popupX + 'px',
        top: popupY + 'px',
      }"
    >
      {{ text }}
    </div>
  </Teleport>
</template>

<style scoped>
/* ════════════════════════════════════════════════════════════════
   宿主容器
   display: contents → 不生成 CSS 盒子，不参与布局
   ════════════════════════════════════════════════════════════════ */

.tooltip-host {
  display: contents;
}

/* ════════════════════════════════════════════════════════════════
   弹出提示窗
   - 无边框，圆角矩形
   - 背景色 var(--colorHover)，文字色 var(--colorTextMain)
   - position: fixed 相对于屏幕视口定位
   - pointer-events: none 防止提示窗挡住鼠标事件
   - z-index 置于所有内容之上
   300ms 延迟出现     │ mouseenter 时启动 setTimeout(show, 300)，mouseleave 时清除 │
   ├────────────────────┼────────────────────────────────────────────────────────────┤
   │ 跟随鼠标           │ mousemove 时实时更新坐标，requestAnimationFrame 防抖       │
   ├────────────────────┼────────────────────────────────────────────────────────────┤
   │ 屏幕边界避让       │ 右溢出翻转到左侧，下溢出翻转到上方，坐标不低于 0           │
   ├────────────────────┼────────────────────────────────────────────────────────────┤
   │ 不限制在应用窗口内 │ <Teleport to="body"> + position: fixed，相对于屏幕视口     │
   ├────────────────────┼────────────────────────────────────────────────────────────┤
   │ 无边框圆角矩形     │ border: none + border-radius: var(--borderRadius)          │
   ├────────────────────┼────────────────────────────────────────────────────────────┤
   │ 颜色               │ 背景 var(--colorHover)，文字 var(--colorTextMain)          │
   ├────────────────────┼────────────────────────────────────────────────────────────┤
   │ 外部输入文本       │ text prop
   ════════════════════════════════════════════════════════════════ */

.tooltip-popup {
  position: fixed;
  z-index: 9999;

  /* 外观 */
  background-color: var(--colorHover);
  color: var(--colorTextMain);
  border: none;
  border-radius: var(--borderRadius);

  /* 内边距 */
  padding: 6px 12px;

  /* 文本 */
  font-size: var(--littleFont);
  line-height: 1.4;
  white-space: nowrap;
  user-select: none;

  /* 确保提示窗本身不拦截鼠标事件（否则会触发父元素的 mouseleave） */
  pointer-events: none;
}
</style>
