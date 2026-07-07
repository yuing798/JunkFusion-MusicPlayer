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
const popupTime = 400 //鼠标悬浮在组件上直到出现的时间
const removeTime = 150 //鼠标移除后悬浮窗多久消失

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
  // window 既不是当前组件，也不是总的 Vue 组件——它是浏览器（或 WebView）提供的全局 JavaScript 对象，
  // 代表当前“浏览器窗口/标签页”这个物理容器，是所有网页组件的“上帝容器”。

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
  cancelTimer() //重置定时器
  showTimer = setTimeout(show, popupTime)
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
  <div
    class="tooltip-host"
    @mouseenter="onMouseEnter"
    @mouseleave="onMouseLeave"
    @mousemove="onMouseMove"
  >
    <slot />
    <!-- <slot /> 是 Vue 的“占位符”——它相当于在子组件里“挖了一个坑”，让父组件可以“填东西”进去。 -->
  </div>

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
/* display: contents; 是一个 CSS 属性值，作用是让元素“隐身但不消失”——它自身不会生成任何盒模型（相当于把自己从布局中移除），
但它的子元素会“顶上来”，就像这个元素不存在一样。 */
.tooltip-host {
  display: contents;
}

.tooltip-popup {
  position: fixed;
  z-index: 9999;
  /* position: fixed; 是“相对于浏览器窗口（视口）固定位置，滚动页面它不动”；z-index: 9999; 是“强行把自己提到最高层，盖住页面上所有其他元素”。 */

  /* 外观 */
  background-color: var(--colorHover);
  color: var(--colorTextMain);
  border: none;
  border-radius: var(--borderRadius);

  /* 内边距 */
  padding: 6px 12px;

  /* 文本 */
  font-size: var(--midFont);
  line-height: 1.4;
  white-space: nowrap;
  /* 1.4 是相对于字体大小的倍数 */
  /* line-height: 1.4; 控制“行与行之间的距离”（行高），white-space: nowrap; 控制“文本不换行”（强制一行显示）。 */

  /* 确保提示窗本身不拦截鼠标事件（否则会触发父元素的 mouseleave） */
  user-select: none;
  pointer-events: none;
  /* pointer-events: none; 就是让元素变成“隐形人”——鼠标事件（点击、悬停、拖动）会直接穿透它，仿佛它根本不存在。 */
}
</style>
