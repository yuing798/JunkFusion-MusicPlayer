<script setup lang="ts">
import { ref, computed, watch } from 'vue';

// ════════════════════════════════════════════════════════════════
// PageChange — 页面切换组件
// 对应 C++ PageChange 类
//
// 使用方式：
//   <PageChange
//     :totalPages="10"
//     :currentPage="1"
//     @page-change="handlePageChange"
//   />
//
// Props：
//   totalPages  — 总页码数
//   currentPage — 当前选中的页码（1 起步）
//
// Emits：
//   page-change — 用户点击了某个页码，传出目标页码
// ════════════════════════════════════════════════════════════════

const props = defineProps<{
  /** 总页码数 */
  totalPages: number;
  /** 当前选中的页码（从 1 开始） */
  currentPage: number;
}>();

const emit = defineEmits<{
  (e: 'page-change', page: number): void;
}>();

// ════════════════════════════════════════════════════════════════
// 跳转输入框
// ════════════════════════════════════════════════════════════════

const jumpInput = ref('');

// 当 totalPages 或 currentPage 变化时清空输入框
watch(
  () => [props.totalPages, props.currentPage],
  // Getter 函数就是“一个返回某个值的函数”。在 Vue 的上下文中，它特指不带参数、只负责返回响应式数据的函数。
  // 它是一个函数（箭头函数或普通函数）
  // 没有参数（通常）
  // 返回一个值
  // 内部访问了响应式数据

  // 箭头函数和普通函数的区别
  // 普通函数：谁调用我，我就指向谁（动态绑定）。
  // 箭头函数：我写在哪里，我就指向哪里（静态绑定）
  // 普通函数：看“点” —— 谁用 . 调用了函数，this 就指向谁。
  // 箭头函数：看“定义” —— 函数在哪个作用域定义的，this 就指向那个作用域的 this。
  () => {
    jumpInput.value = '';
  },
  // 为什么这里都写箭头函数
  // 箭头函数：() => [...]（一行搞定，不用写 return）
  // 普通函数：function() { return [...]; }（要写两行）
  // 如果你确实需要用到 this 来访问组件实例（比如在 Options API 中），那你必须用普通函数，
  // 因为箭头函数会绑定外层的 this（通常是 undefined），拿不到组件实例。
  // 但在 <script setup> 中，你永远不需要用 this，所以箭头函数就是绝对的最佳选择。
);

/** 点击 go 按钮或按下回车 */
function goAndEnterClick(): void {
  const targetPage = parseInt(jumpInput.value, 10); //将输入框的输入解析为10进制
  if (isNaN(targetPage)) return;
  if (targetPage >= 1 && targetPage <= props.totalPages && targetPage !== props.currentPage) {
    jumpInput.value = ''; //输入数字有效并回车开始跳转
    emit('page-change', targetPage);
  }
}

/** 输入框回车 */
function onJumpKeydown(e: KeyboardEvent): void {
  if (e.key === 'Enter') {
    goAndEnterClick();
  }
}

// ════════════════════════════════════════════════════════════════
// 页码按钮布局
// 对应 C++ PageChange::doLayout()
// 规则：
//   ≤ 7 页 → 全部显示
//   > 7 页 → [1] + 中间区域(5槽) + [N]，根据 currentPage 动态计算
// ════════════════════════════════════════════════════════════════

interface PageItem {
  type: 'page' | 'ellipsis';
  page?: number; //? 表示这个字段是可选的，这里的 page 是变量名（对象的属性名）
}

const pageItems = computed<PageItem[]>(() => {
  // 这里的 <PageItem[]> 就是给 computed 加上的泛型（Generic）类型标注，
  // 它的意思是：这个计算属性返回的值，其类型是 PageItem 类型的数组（即 PageItem[]）。
  // 泛型应该写在 computed 上（computed<类型>(...)），而不是写在变量名上。因为 computed() 返回的是 ComputedRef，不是原始值。
  // 也就是说
  // const pageItems : PageItem[] = computed(() =>{})是错误的
  const n = props.totalPages;
  const cur = props.currentPage;

  if (n <= 0) return [];

  if (n <= 7) {
    // 全部显示
    //生成了一个长度为 n 的数组，每个元素都是一个带有 type 和 page 属性的对象，分别表示“页码类型”和“页码数值”。
    //(_, i) => ({ type: 'page' as const, page: i + 1 })（映射函数）
    return Array.from({ length: n }, (_, i) => ({ type: 'page' as const, page: i + 1 }));
  }

  // > 7 页：[1] + 5 槽 + [N]
  const items: PageItem[] = [];
  items.push({ type: 'page', page: 1 });

  let winStart: number;
  let winEnd: number;

  if (cur <= 3) {
    // 靠近首页
    winStart = 2;
    winEnd = 5;
  } else if (cur >= n - 2) {
    // 靠近末页
    winEnd = n - 1;
    winStart = n - 4;
  } else {
    // 中间
    winStart = cur - 1;
    winEnd = cur + 1;
  }

  if (winStart > 2) {
    items.push({ type: 'ellipsis' });
  }

  for (let p = winStart; p <= winEnd; ++p) {
    items.push({ type: 'page', page: p });
  }

  if (winEnd < n - 1) {
    items.push({ type: 'ellipsis' });
  }

  items.push({ type: 'page', page: n });

  return items;
});

// ════════════════════════════════════════════════════════════════
// 按钮禁用状态
// ════════════════════════════════════════════════════════════════

const isPrevDisabled = computed<boolean>(() => props.currentPage <= 1);
const isNextDisabled = computed<boolean>(() => props.currentPage >= props.totalPages);

// ════════════════════════════════════════════════════════════════
// 事件处理
// ════════════════════════════════════════════════════════════════

function goToPage(page: number): void {
  if (page >= 1 && page <= props.totalPages && page !== props.currentPage) {
    emit('page-change', page);
  }
}

function goPrev(): void {
  if (props.currentPage > 1) {
    emit('page-change', props.currentPage - 1);
  }
}

function goNext(): void {
  if (props.currentPage < props.totalPages) {
    emit('page-change', props.currentPage + 1);
  }
}

/** 判断是否为当前页 */
function isCurrent(page: number): boolean {
  return page === props.currentPage;
}
</script>

<template>
  <!--
    PageChange — 页面切换组件
    对应 C++ PageChange 类，总尺寸 360×80
    第 1 行：居中排列的页码按钮 (<, 1, 2, 3, ..., N, >)
    第 2 行：跳转输入框 + go 按钮
  -->
  <div class="page-change">
    <!-- ═══════════════════════════════════════════════════════════
         第 1 行：页码导航行（高度 40px）
         ═══════════════════════════════════════════════════════════ -->
    <div class="page-row">
      <!-- < 上一页，对应 C++ previousButton -->
      <button
        class="page-btn"
        :class="{ disabled: isPrevDisabled }"
        :disabled="isPrevDisabled"
        @click="goPrev()"
      >
        <
      </button>

      <!-- 页码 / 省略号，对应 C++ buttons / ellipsisLabels -->
      <template v-for="(item, idx) in pageItems" :key="idx">
        <button
          v-if="item.type === 'page'"
          class="page-btn"
          :class="{ active: isCurrent(item.page!) }"
          @click="goToPage(item.page!)"
        >
          {{ item.page }}
        </button>
        <span v-else class="ellipsis"> ... </span>
      </template>

      <!-- > 下一页，对应 C++ nextButton -->
      <button
        class="page-btn"
        :class="{ disabled: isNextDisabled }"
        :disabled="isNextDisabled"
        @click="goNext"
      >
        >
      </button>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 2 行：跳转行（高度 40px）
         输入框 160px + go 按钮 40px
         ═══════════════════════════════════════════════════════════ -->
    <div class="jump-row">
      <div class="jump-spacer" />
      <input
        v-model="jumpInput"
        class="jump-input"
        type="text"
        placeholder="跳转到..."
        @keydown="onJumpKeydown"
      />
      <!-- v-model 是 Vue 中双向数据绑定的核心指令。简单来说，它让 输入框的值 和 JavaScript 变量 之间建立了一个“实时同步通道”。 -->
      <button class="page-btn go-btn" @click="goAndEnterClick">go</button>
    </div>
  </div>
</template>

<style scoped>
/* ════════════════════════════════════════════════════════════════
   PageChange 容器
   对应 C++: setSize(360, 80)
   ════════════════════════════════════════════════════════════════ */

.page-change {
  display: flex;
  flex-direction: column;
  width: 360px;
  height: 80px;
  user-select: none;
}

/* ════════════════════════════════════════════════════════════════
   第 1 行：页码导航
   居中排列所有按钮，每个 40×40
   ════════════════════════════════════════════════════════════════ */

.page-row {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 40px;
  gap: 0;
}

/* ── 页码按钮（<, 1, 2, ..., N, >） ── */
.page-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  margin: 2px;

  border: none;
  border-radius: var(--border-radius);
  background-color: var(--color-main);
  color: var(--color-text-main);
  font-size: var(--mid-font);
  cursor: pointer;
  transition:
    background-color var(--ease-time) ease,
    color var(--ease-time) ease;
}

.page-btn:hover:not(.disabled, .active) {
  background-color: var(--color-hover);
}

/* 当前页 — 对应 C++ setPageButtonColor: blackGrey 背景 + 白字 */
.page-btn.active {
  background-color: var(--color-text-second);
  color: #ffffff;
}

/* 禁用态（< 在第一页时, > 在最后一页时） */
.page-btn.disabled {
  opacity: 0.3;
  cursor: default;
  pointer-events: none;
}

/* ── 省略号 ── */
.ellipsis {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  font-size: var(--mid-font);
  color: var(--color-text-second);
  cursor: default;
}

/* ════════════════════════════════════════════════════════════════
   第 2 行：跳转行
   ════════════════════════════════════════════════════════════════ */

.jump-row {
  display: flex;
  align-items: center;
  height: 40px;
}

/* 左侧弹性空间，让输入框靠右（对应 C++ row2.removeFromLeft(80)） */
.jump-spacer {
  width: 80px;
  flex-shrink: 0;
}

/* 输入框：对应 C++ YTextEditor, setSize(170, 40) → reduced(5) → 有效宽度 160px */
.jump-input {
  width: 160px;
  height: 30px;
  padding: 0 8px;
  margin: 0 5px;

  border: 3px solid var(--color-edge);
  border-radius: var(--border-radius);
  background-color: transparent;
  color: var(--color-text-main);
  font-size: var(--mid-font);
  outline: none;
  transition: border-color var(--ease-time) ease;
}

.jump-input:hover {
  border-color: var(--color-text-second);
}

.jump-input:focus {
  border-color: var(--color-stress);
}

.jump-input::placeholder {
  color: var(--color-text-second);
  opacity: 0.6;
}

/* go 按钮 */
.go-btn {
  flex-shrink: 0;
}
</style>
