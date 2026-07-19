<script setup lang="ts">
import { ref, nextTick, onUnmounted } from 'vue'
import { IconX } from '@tabler/icons-vue'

// ════════════════════════════════════════════════════════════════
// PopupWindow — 可复用的弹出窗组件
//
// 使用方式：
//   <PopupWindow ref="popupRef" title="歌曲详情">
//     <template #trigger-button>
//       <button>打开弹窗</button>
//     </template>
//     <template #popup-window-component>
//       <YourContentComponent />
//     </template>
//   </PopupWindow>
//
// 父组件中调用 popupRef.open() / popupRef.close() 来控制弹窗。
//
// 特性：
//   - 点击 #trigger-button 插槽内容自动打开弹窗
//   - 缩放 + 淡入动画，变换原点 = 触发按钮中心
//   - 标题居中，右上角 X 关闭按钮
//   - 宽高由 #popup-window-component 内容自然决定，标题栏高度固定 32px
//   - Teleport to body
// ════════════════════════════════════════════════════════════════

const props = defineProps<{
  /** 弹窗标题（显示在标题栏居中位置） */
  title: string
}>()

const emit = defineEmits<{
  (e: 'close'): void
}>()

// ── 状态 ──

const triggerWrapper = ref<HTMLElement | null>(null)
const popupEl = ref<HTMLElement | null>(null)
const visible = ref(false)
const closing = ref(false)
const popupLeft = ref('0px')
const popupTop = ref('0px')

// ── 打开 / 关闭 ──

/**
 * 打开弹窗。
 *
 * 1. 将 CSS 自定义属性 --popup-origin-x/y 设为触发按钮的中心坐标，
 *    使 scale 动画以该点为原点展开。
 * 2. 渲染后根据弹窗实际尺寸精确居中。
 */
function open(): void {
  if (triggerWrapper.value) {
    const rect = triggerWrapper.value.getBoundingClientRect()
    document.documentElement.style.setProperty(
      '--popup-origin-x',
      rect.left + rect.width / 2 + 'px',
    )
    document.documentElement.style.setProperty(
      '--popup-origin-y',
      rect.top + rect.height / 2 + 'px',
    )
  }

  visible.value = true
  closing.value = false

  // 渲染后根据实际尺寸精确居中
  void nextTick(() => {
    if (popupEl.value) {
      const r = popupEl.value.getBoundingClientRect()
      popupLeft.value = (window.innerWidth - r.width) / 2 + 'px'
      popupTop.value = (window.innerHeight - r.height) / 2 + 'px'
    }
  })
}

/** 关闭弹窗（带动画） */
function close(): void {
  closing.value = true
  setTimeout(() => {
    visible.value = false
    closing.value = false
    emit('close')
  }, 200) // 匹配 CSS 动画时长，延迟执行
}

// ── 暴露给父组件 ──

defineExpose({ open, close })

// ── 清理 ──

onUnmounted(() => {
  document.documentElement.style.removeProperty('--popup-origin-x')
  document.documentElement.style.removeProperty('--popup-origin-y')
})
</script>

<template>
  <!--
    触发区域：包裹 #trigger-button 插槽，点击时打开弹窗。
    使用 <span> 以便作为内联元素融入父级布局（如 Grid cell）。
  -->
  <span ref="triggerWrapper" class="popup-trigger" @click.stop="open">
    <slot name="trigger-button" />
  </span>

  <!-- 弹出窗 — Teleport to body -->
  <Teleport to="body">
    <div
      v-if="visible"
      ref="popupEl"
      class="popup-window"
      :class="{ 'popup-closing': closing }"
      :style="{
        left: popupLeft,
        top: popupTop,
      }"
    >
      <!-- 标题栏：居中标题 + 右侧关闭按钮 -->
      <div class="popup-titlebar">
        <span class="popup-title">{{ title }}</span>
        <button class="popup-close-btn" @click="close">
          <IconX :size="18" />
        </button>
      </div>

      <!-- 内容区域：由调用方投射任意组件，宽高由此插槽内容自然决定 -->
      <div class="popup-body">
        <slot name="popup-window-component" />
      </div>
    </div>
  </Teleport>
</template>

<style scoped>
/* ════════════════════════════════════════════════════════════════
   触发区域 — 内联包裹，不生成盒模型
   ════════════════════════════════════════════════════════════════ */

.popup-trigger {
  display: contents;
}

/* ════════════════════════════════════════════════════════════════
   弹出窗 — Teleported to body
   仿 C++ popupWindow + PopupWindowButton 的缩放动画
   宽高由内容决定，不设 width/height
   ════════════════════════════════════════════════════════════════ */

.popup-window {
  position: fixed;
  z-index: 10000;
  /* 宽高由 .popup-titlebar(32px) + .popup-body 内容自适应 */
  background-color: var(--color-hover);
  border-radius: var(--border-radius);
  box-shadow: 0 4px 24px rgb(0, 0, 0, 25%);

  /*
    变换原点通过 JS 动态设置（触发按钮中心在 viewport 中的坐标），
    模拟 C++ AffineTransform::scale(progress, progress, cx, cy)
  */
  transform-origin: var(--popup-origin-x, center) var(--popup-origin-y, center);

  /* v-if 挂载时自动触发打开动画 */
  animation: popup-open 200ms ease-out forwards;
}

.popup-window.popup-closing {
  animation: popup-close 200ms ease-in forwards;
}

@keyframes popup-open {
  from {
    transform: scale(0);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}

@keyframes popup-close {
  from {
    transform: scale(1);
    opacity: 1;
  }
  to {
    transform: scale(0);
    opacity: 0;
  }
}

/* ── 标题栏 ── */

.popup-titlebar {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 32px;
  padding: 0 8px;
  position: relative;
  user-select: none;
}

.popup-title {
  font-size: var(--mid-font);
  font-weight: bold;
  color: var(--color-text-main);
  max-width: calc(100% - 32px);
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
}

.popup-close-btn {
  position: absolute;
  right: 4px;
  top: 50%;
  transform: translateY(-50%);
  width: 24px;
  height: 24px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: none;
  background: transparent;
  color: var(--color-text-second);
  cursor: pointer;
  border-radius: var(--border-radius);
  padding: 0;
  transition:
    background-color var(--ease-time) ease,
    color var(--ease-time) ease;
}

.popup-close-btn:hover {
  background-color: var(--color-hover);
  color: var(--color-text-main);
}
</style>
