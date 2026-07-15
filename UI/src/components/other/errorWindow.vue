<script setup lang="ts">
import { ref } from 'vue';
import { IconX } from '@tabler/icons-vue';
// ════════════════════════════════════════════════════════════════
// 模块级单例状态
//
// 所有导入此模块的 .ts/.vue 文件共享同一份 message/visible 实例，
// 因此任意位置的 try/catch 调用 showErrorWindow() 都能驱动同一个弹窗。
// ════════════════════════════════════════════════════════════════

/** 弹窗显示的文本内容 */
const message = ref('');

/** 是否正在显示（驱动 CSS transition） */
const visible = ref(false);

/** 隐藏定时器句柄，用于在新错误到来时重置计时 */
let hideTimer: ReturnType<typeof setTimeout> | null = null;

/** 从任意类型的错误中提取可显示的字符串 */
function extractError(error: unknown): string {
  if (error instanceof Error) {
    return error.message;
  }
  if (typeof error === 'string') {
    return error;
  }
  if (error !== null && error !== undefined) {
    return error.toString();
  }
  return '';
}

/**
 * 立刻关闭弹窗（被用户点击 X 按钮或是手动调用时触发）。
 * 不经过定时器，直接触发 300ms 淡出动画。
 */
export function closeErrorWindow(): void {
  if (hideTimer !== null) {
    clearTimeout(hideTimer);
    hideTimer = null;
  }
  visible.value = false;
}

/**
 * 全局错误弹窗函数。
 *
 * 使用方式（任意 .ts / .vue 文件）：
 * ```ts
 * import { showErrorWindow } from '@/components/other/errorWindow.vue'
 *
 * try {
 *   // ...
 * } catch (error) {
 *   showErrorWindow(error)
 * }
 *
 * // 自定义保持时间：
 * showErrorWindow('操作成功', 2000)
 * showErrorWindow(new Error('失败'), 5000)
 * ```
 *
 * @param msg  - 错误信息。支持 Error / string / 任意对象（自动 JSON.stringify）
 * @param holdTime - 弹窗保持的时长（毫秒），不含淡入淡出。默认 3000ms
 *
 * 行为：
 * - 弹窗以 300ms 淡入动画出现在窗口正中央
 * - 保持 holdTime 毫秒
 * - 300ms 淡出后消失
 * - 用户可点击右上角 X 按钮提前关闭
 * - 连续调用时重置计时器，始终展示最新一条错误
 */
export function showErrorWindow(msg: unknown, holdTime: number = 3000): void {
  if (hideTimer !== null) {
    clearTimeout(hideTimer);
    hideTimer = null;
  } // 清理已有的定时器，防止重合

  message.value = extractError(msg);
  visible.value = true;

  // 300ms（淡入）+ holdTime（保持）后开始淡出
  hideTimer = setTimeout(() => {
    visible.value = false;
    hideTimer = null;
  }, 300 + holdTime);
  // lambda 函数在当计时器结束时会被执行
}
</script>

<template>
  <Teleport to="body">
    <div class="error-popup" :class="{ visible }">
      <div class="error-popup-content">
        {{ message }}
      </div>
      <button class="error-popup-close" @click="closeErrorWindow">
        <IconX class="svg-button" />
      </button>
    </div>
  </Teleport>
</template>

<style scoped>
/* ════════════════════════════════════════════════════════════════
   error-popup — 全局错误弹窗

   动画时序：
     - 0ms     → 300ms      opacity 0 → 1（淡入）
     - 300ms   → 300+hold   opacity 1（保持）
     - 然后               opacity 1 → 0（淡出，300ms）
   ════════════════════════════════════════════════════════════════ */
.svg-button {
  width: 30px;
  height: 30px;
  color: var(--color-text-main);
}

.error-popup {
  position: fixed;
  /* “钉在屏幕上的元素”，无论页面如何滚动，它都待在原地不动。 */
  z-index: 10000;
  left: 50%;
  /* 把元素的左侧移到父容器宽度的50%位置 */
  top: 50%;
  transform: translate(-50%, -50%);
  /* 把元素向左移动自身宽度的50%，向上移动自身高度的50% */

  /* 固定宽度，高度由内容撑开 */
  width: 360px;

  /* 外观 */
  background-color: color-mix(in srgb, var(--color-hover), transparent 30%);
  /* in srgb:请用 sRGB 这个坐标系的规则来计算两种颜色的中间值 */
  border-radius: var(--border-radius);
  box-shadow: 0 4px 16px rgb(0, 0, 0, 25%);

  /* 默认隐藏 — 由 .visible 控制淡入/淡出 */
  opacity: 0;
  /* 0代表完全不透明 */
  transition: opacity var(--ease-time) ease;
  /* pointer-events: none; 会让当前元素及其所有子元素完全"不响应"鼠标事件（点击、悬停、拖拽等）。
  即使给子元素单独设置 pointer-events: auto;，也无效，因为父级的规则会阻止事件到达子元素。 */
}

.error-popup.visible {
  opacity: 1;
}

/* ── 内容区域 ── */
.error-popup-content {
  color: var(--color-error);
  font-size: var(--mid-font);
  line-height: 1.5;
  text-align: center;
  padding: 16px 24px;
}

/* ── 关闭按钮（右上角 X） ── */
.error-popup-close {
  position: absolute;
  top: 6px;
  right: 6px;
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: none;
  background: transparent;
  color: var(--color-text-main);
  cursor: pointer;
  border-radius: var(--border-radius);
  padding: 0;
  transition: background-color var(--ease-time) ease;

  pointer-events: auto;
}

.error-popup-close:hover {
  background-color: color-mix(in srgb, var(--color-hover), transparent 0%);
}
</style>
