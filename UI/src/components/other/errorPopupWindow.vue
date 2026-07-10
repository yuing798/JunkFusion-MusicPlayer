<script lang="ts">
import { ref } from 'vue'

// ════════════════════════════════════════════════════════════════
// 模块级单例状态
//
// 所有导入此模块的 .ts/.vue 文件共享同一份 message/visible 实例，
// 因此任意位置的 try/catch 调用 showErrorPopup() 都能驱动同一个弹窗。
// ════════════════════════════════════════════════════════════════

/** 弹窗显示的文本内容 */
const message = ref('')

/** 是否正在显示（驱动 CSS transition） */
const visible = ref(false)

/** 隐藏定时器句柄，用于在新错误到来时重置计时 */
let hideTimer: ReturnType<typeof setTimeout> | null = null

/** 从任意类型的错误中提取可显示的字符串 */
function extractError(error: unknown): string {
  if (error instanceof Error) {
    return error.message
  }
  if (typeof error === 'string') {
    return error
  }
  if (error !== null && error !== undefined) {
    try {
      return JSON.stringify(error)
    } catch {
      return String(error)
    }
  }
  return ''
}

/**
 * 立刻关闭弹窗（被用户点击 X 按钮或是手动调用时触发）。
 * 不经过定时器，直接触发 300ms 淡出动画。
 */
export function closeErrorPopup(): void {
  if (hideTimer !== null) {
    clearTimeout(hideTimer)
    hideTimer = null
  }
  visible.value = false
}

/**
 * 全局错误弹窗函数。
 *
 * 使用方式（任意 .ts / .vue 文件）：
 * ```ts
 * import { showErrorPopup } from '@/components/other/errorPopupWindow.vue'
 *
 * try {
 *   // ...
 * } catch (error) {
 *   showErrorPopup(error)
 * }
 *
 * // 自定义保持时间：
 * showErrorPopup('操作成功', 2000)
 * showErrorPopup(new Error('失败'), 5000)
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
export function showErrorPopup(msg: unknown, holdTime: number = 3000): void {
  if (hideTimer !== null) {
    clearTimeout(hideTimer)
    hideTimer = null
  } // 清理已有的定时器，防止重合

  message.value = extractError(msg)
  visible.value = true

  // 300ms（淡入）+ holdTime（保持）后开始淡出
  hideTimer = setTimeout(() => {
    visible.value = false
    hideTimer = null
  }, 300 + holdTime)
  // lambda 函数在当计时器结束时会被执行
}
</script>

<script setup lang="ts">
import { IconX } from '@tabler/icons-vue'
// ── 组件本身仅负责渲染，所有状态由模块级 <script> 驱动 ──
</script>

<template>
  <Teleport to="body">
    <div class="error-popup" :class="{ visible }">
      <div class="error-popup__content">
        {{ message }}
      </div>
      <button class="error-popup__close" @click="closeErrorPopup">
        <IconX :size="18" />
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

.error-popup {
  position: fixed;
  z-index: 10000;
  left: 50%;
  top: 50%;
  transform: translate(-50%, -50%);

  /* 固定宽度，高度由内容撑开 */
  width: 360px;

  /* 外观 */
  background-color: color-mix(in srgb, var(--colorHover), transparent 30%);
  border-radius: var(--borderRadius);
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.25);

  /* 默认隐藏 — 由 .visible 控制淡入/淡出 */
  opacity: 0;
  transition: opacity 300ms ease;
  pointer-events: none;
}

.error-popup.visible {
  opacity: 1;
  /*
     pointer-events 保持 none，整个弹窗不阻挡下层鼠标事件 —
     只有右上角 X 按钮单独恢复为 auto，保证关闭按钮可点击
  */
}

/* ── 内容区域 ── */
.error-popup__content {
  color: var(--colorError);
  font-size: var(--midFont);
  line-height: 1.5;
  text-align: center;
  padding: 16px 24px;
  word-break: break-word;
}

/* ── 关闭按钮（右上角 X） ── */
.error-popup__close {
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
  color: var(--colorTextSecond);
  cursor: pointer;
  border-radius: var(--borderRadius);
  padding: 0;
  transition: background-color var(--easeTime) ease;

  /* 只让这个按钮响应鼠标，下层其余部分继续穿透 */
  pointer-events: auto;
}

.error-popup__close:hover {
  background-color: color-mix(in srgb, var(--colorHover), transparent 0%);
  color: var(--colorTextMain);
}
</style>
