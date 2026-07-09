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
 *   showErrorPopup(error instanceof Error ? error.message : String(error))
 * }
 * ```
 *
 * 行为：
 * - 弹窗以 300ms 淡入动画出现在窗口正中央
 * - 保持 3 秒
 * - 300ms 淡出后消失
 * - 连续调用时重置计时器，始终展示最新一条错误
 */
export function showErrorPopup(msg: unknown): void {
  if (hideTimer !== null) {
    clearTimeout(hideTimer)
    hideTimer = null
  } //清理已有的定时器，防止重合

  const extractError = (error: unknown): string => {
    if (error instanceof Error) {
      return error.message
    }
    if (typeof error === 'string') {
      return error
    }
    if (error !== null && error != undefined) {
      try {
        return JSON.stringify(error)
      } catch {
        return String(error)
      }
    }
    return ''
  }

  message.value = extractError(msg)
  visible.value = true

  // 300ms 淡入 + 3000ms 保持 = 3300ms 后开始淡出
  hideTimer = setTimeout(() => {
    visible.value = false
    hideTimer = null
  }, 3300)
  //lambda函数在当计时器结束时会被执行
}
</script>

<script setup lang="ts">
// ── 组件本身仅负责渲染，所有状态由模块级 <script> 驱动 ──
</script>

<template>
  <Teleport to="body">
    <div class="error-popup" :class="{ visible }">
      <div class="error-popup__content">
        {{ message }}
      </div>
    </div>
  </Teleport>
</template>

<style scoped>
/* ════════════════════════════════════════════════════════════════
   error-popup — 全局错误弹窗

   动画时序：
     - 0ms     → 300ms    opacity 0 → 1（淡入）
     - 300ms   → 3300ms   opacity 1（保持）
     - 3300ms  → 3600ms   opacity 1 → 0（淡出）
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
  /* pointer-events 保持 none，避免遮挡用户操作 */
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
</style>
