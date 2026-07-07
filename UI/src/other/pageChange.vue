<script setup lang="ts">
import { ref, computed, watch } from 'vue'

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
  totalPages: number
  /** 当前选中的页码（从 1 开始） */
  currentPage: number
}>()

const emit = defineEmits<{
  (e: 'page-change', page: number): void
}>()

// ════════════════════════════════════════════════════════════════
// 跳转输入框
// ════════════════════════════════════════════════════════════════

const jumpInput = ref('')

// 当 totalPages 或 currentPage 变化时清空输入框
watch(() => [props.totalPages, props.currentPage], () => {
  jumpInput.value = ''
})

/** 点击 go 按钮或按下回车 */
function goAndEnterClick(): void {
  const targetPage = parseInt(jumpInput.value, 10)
  if (isNaN(targetPage)) return
  if (targetPage >= 1 && targetPage <= props.totalPages && targetPage !== props.currentPage) {
    jumpInput.value = ''
    emit('page-change', targetPage)
  }
}

/** 输入框回车 */
function onJumpKeydown(e: KeyboardEvent): void {
  if (e.key === 'Enter') {
    goAndEnterClick()
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
  type: 'page' | 'ellipsis'
  page?: number
}

const pageItems = computed<PageItem[]>(() => {
  const n = props.totalPages
  const cur = props.currentPage

  if (n <= 0) return []

  if (n <= 7) {
    // 全部显示
    return Array.from({ length: n }, (_, i) => ({ type: 'page' as const, page: i + 1 }))
  }

  // > 7 页：[1] + 5 槽 + [N]
  const items: PageItem[] = []
  items.push({ type: 'page', page: 1 })

  let winStart: number
  let winEnd: number

  if (cur <= 3) {
    // 靠近首页
    winStart = 2
    winEnd = 5
  } else if (cur >= n - 2) {
    // 靠近末页
    winEnd = n - 1
    winStart = n - 4
  } else {
    // 中间
    winStart = cur - 1
    winEnd = cur + 1
  }

  if (winStart > 2) {
    items.push({ type: 'ellipsis' })
  }

  for (let p = winStart; p <= winEnd; ++p) {
    items.push({ type: 'page', page: p })
  }

  if (winEnd < n - 1) {
    items.push({ type: 'ellipsis' })
  }

  items.push({ type: 'page', page: n })

  return items
})

// ════════════════════════════════════════════════════════════════
// 按钮禁用状态
// ════════════════════════════════════════════════════════════════

const isPrevDisabled = computed(() => props.currentPage <= 1)
const isNextDisabled = computed(() => props.currentPage >= props.totalPages)

// ════════════════════════════════════════════════════════════════
// 事件处理
// ════════════════════════════════════════════════════════════════

function goToPage(page: number): void {
  if (page >= 1 && page <= props.totalPages && page !== props.currentPage) {
    emit('page-change', page)
  }
}

function goPrev(): void {
  if (props.currentPage > 1) {
    emit('page-change', props.currentPage - 1)
  }
}

function goNext(): void {
  if (props.currentPage < props.totalPages) {
    emit('page-change', props.currentPage + 1)
  }
}

/** 判断是否为当前页 */
function isCurrent(page: number): boolean {
  return page === props.currentPage
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
        @click="goPrev"
      >
        &lt;
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
        <span v-else class="ellipsis">
          ...
        </span>
      </template>

      <!-- > 下一页，对应 C++ nextButton -->
      <button
        class="page-btn"
        :class="{ disabled: isNextDisabled }"
        :disabled="isNextDisabled"
        @click="goNext"
      >
        &gt;
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
      <button class="page-btn go-btn" @click="goAndEnterClick">
        go
      </button>
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
  border-radius: var(--borderRadius);
  background-color: var(--colorMain);
  color: var(--colorTextMain);
  font-size: var(--midFont);
  cursor: pointer;
  transition:
    background-color var(--easeTime) ease,
    color var(--easeTime) ease;
}

.page-btn:hover:not(.disabled):not(.active) {
  background-color: var(--colorHover);
}

/* 当前页 — 对应 C++ setPageButtonColor: blackGrey 背景 + 白字 */
.page-btn.active {
  background-color: var(--colorTextSecond);
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
  font-size: var(--midFont);
  color: var(--colorTextSecond);
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

  border: 3px solid var(--colorEdge);
  border-radius: var(--borderRadius);
  background-color: transparent;
  color: var(--colorTextMain);
  font-size: var(--midFont);
  outline: none;
  transition: border-color var(--easeTime) ease;
}

.jump-input:hover {
  border-color: var(--colorTextSecond);
}

.jump-input:focus {
  border-color: var(--colorStress);
}

.jump-input::placeholder {
  color: var(--colorTextSecond);
  opacity: 0.6;
}

/* go 按钮 */
.go-btn {
  flex-shrink: 0;
}
</style>
