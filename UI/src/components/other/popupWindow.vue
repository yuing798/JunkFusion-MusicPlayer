<script setup lang="ts">
import { ref, nextTick, onUnmounted } from 'vue';
import { IconX } from '@tabler/icons-vue';

// ════════════════════════════════════════════════════════════════
// PopupWindow — 可复用的弹出窗组件
//
// 使用方式：
//   <PopupWindow ref="popupRef" title="歌曲详情" >
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
//   - 可拖动标题栏
//   - 标题居中，右上角 X 关闭按钮
//   - Teleport to body
// ════════════════════════════════════════════════════════════════

const props = defineProps<{
  /** 弹窗标题（显示在标题栏居中位置） */
  title: string;
  /** 弹窗宽度（px） */
  width: number;
}>();

const emit = defineEmits<{
  (e: 'close'): void;
}>();

// ── 状态 ──

const triggerWrapper = ref<HTMLElement | null>(null);
const popupEl = ref<HTMLElement | null>(null);
const visible = ref(false);
const closing = ref(false);
const popupLeft = ref('0px');
const popupTop = ref('0px');

// ── 拖动 ──

const dragging = ref(false);
const dragOffset = ref({ x: 0, y: 0 });

function onTitleMouseDown(e: MouseEvent): void {
  if ((e.target as HTMLElement).closest('.popup-close-btn')) return;
  // closest() 是 DOM 的原生方法，从当前元素开始，向父级查找匹配选择器的元素
  dragging.value = true;
  const rect = popupEl.value!.getBoundingClientRect();
  // 获得popupEl这个元素在整个应用中的宽高和相对坐标
  dragOffset.value = { x: e.clientX - rect.left, y: e.clientY - rect.top };
  // 获得鼠标在当前组件坐标系的坐标
  window.addEventListener('mousemove', onDragMove);
  window.addEventListener('mouseup', onDragEnd);
}

function onDragMove(e: MouseEvent): void {
  if (!dragging.value) return;
  popupLeft.value = e.clientX - dragOffset.value.x + 'px';
  popupTop.value = e.clientY - dragOffset.value.y + 'px';
}

function onDragEnd(): void {
  dragging.value = false;
  window.removeEventListener('mousemove', onDragMove);
  window.removeEventListener('mouseup', onDragEnd);
}

// ── 打开 / 关闭 ──

/**
 * 打开弹窗。
 *
 * 1. 将 CSS 自定义属性 --popup-origin-x/y 设为触发按钮的中心坐标，
 *    使 scale 动画以该点为原点展开。
 * 2. 初始粗略居中，渲染后根据实际尺寸精确居中。
 */
function open(): void {
  if (triggerWrapper.value) {
    // triggerWrapper 是内联 <span>，其 getBoundingClientRect 即为内容的包围盒
    const rect = triggerWrapper.value.getBoundingClientRect();
    document.documentElement.style.setProperty(
      '--popup-origin-x',
      rect.left + rect.width / 2 + 'px',
    );
    document.documentElement.style.setProperty(
      '--popup-origin-y',
      rect.top + rect.height / 2 + 'px',
    );
  }

  visible.value = true;
  closing.value = false;

  // 粗略居中
  popupLeft.value = (window.innerWidth - props.width) / 2 + 'px';
  popupTop.value = (window.innerHeight - 350) / 2 + 'px';

  // 渲染后精确居中
  void nextTick(() => {
    if (popupEl.value) {
      const r = popupEl.value.getBoundingClientRect();
      popupLeft.value = (window.innerWidth - r.width) / 2 + 'px';
      popupTop.value = (window.innerHeight - r.height) / 2 + 'px';
    }
  });
}

/** 关闭弹窗（带动画） */
function close(): void {
  closing.value = true;
  setTimeout(() => {
    visible.value = false;
    closing.value = false;
    emit('close');
  }, 200); // 匹配 CSS 动画时长，延迟执行
}

// ── 暴露给父组件 ──

defineExpose({ open, close });
// emit：是子组件向父组件“喊话”（向上传递事件）。
// defineExpose：是父组件主动“伸手”去拿子组件的东西（向下调用方法）。
// defineExpose：“子组件把钥匙交给父组件”，父组件通过 ref 直接调用。
// export：“组件把工具放在工具箱里”，任何 JS 文件都可以 import 拿走去用。

// ── 清理 ──

onUnmounted(() => {
  document.documentElement.style.removeProperty('--popup-origin-x');
  document.documentElement.style.removeProperty('--popup-origin-y');
  window.removeEventListener('mousemove', onDragMove);
  window.removeEventListener('mouseup', onDragEnd);
});
</script>

<template>
  <!--
    触发区域：包裹 #trigger 插槽，点击时打开弹窗。
    使用 <span> 以便作为内联元素融入父级布局（如 Grid cell）。
  -->
  <span ref="triggerWrapper" class="popup-trigger" @click.stop="open">
    <!-- ref的意思是绑定js对象使得该对象能够实现函数效果 -->
    <!-- 父组件调用<templete #插槽名>来进行填充 -->
    <slot name="trigger-button" />
    <!-- 向外连接按钮，点击出现弹窗 -->
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
      <!-- 标题栏：居中标题 + 右侧关闭按钮，可拖动 -->
      <div class="popup-titlebar" @mousedown="onTitleMouseDown">
        <span class="popup-title">{{ title }}</span>
        <button class="popup-close-btn" @click="close">
          <IconX :size="18" />
        </button>
      </div>

      <!-- 内容区域：默认插槽，由调用方投射任意组件 -->
      <div class="popup-body">
        <slot name="popup-window-component" />
        <!-- 默认插槽，没有内容 -->
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
  /* display: contents; 就像把这个元素从 DOM 树中“掏空”，只留下它的子元素，仿佛这个元素从未存在过。 */
}

/* ════════════════════════════════════════════════════════════════
   弹出窗 — Teleported to body
   仿 C++ popupWindow + PopupWindowButton 的缩放动画
   ════════════════════════════════════════════════════════════════ */

.popup-window {
  position: fixed;
  /* 这个元素会相对于“浏览器可视区域（Viewport）”固定，不会随页面滚动而移动。 但它本身可以通过 top、left、transform 等属性移动位置。 */
  z-index: 10000;
  min-height: 250px;
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
/* 内联元素（Inline Element） 就是“不换行”的元素。它只占据自己内容所需的宽度，多个内联元素会在同一行从左到右依次排列，直到宽度不够才换行。 */
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
  cursor: move;
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

/* ── 内容区域 ── */

.popup-body {
  padding: 5px;
}
</style>
