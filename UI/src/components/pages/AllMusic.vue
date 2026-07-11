<script setup lang="ts">
import { onMounted, ref } from 'vue';

import { callJuceFunc } from '@/bridge/bridgeSupport.ts';
import { BRIDGE_inputFiles } from '@/bridge/bridge.generated.ts';
import { IconArrowBigDownFilled, IconArrowBigUpFilled } from '@tabler/icons-vue';

const songCount = ref(0);

const sortOptions = [
  { value: 0, label: '添加时间' }, // SortMode::ByAddTime
  { value: 1, label: '歌曲名称' }, // SortMode::ByName
  { value: 2, label: '播放次数' }, // SortMode::ByPlayTimes
];
const selectedSort = ref(0);
// 映射到 SortMode 枚举：与 C++ indexToSortMode lambda 一致
function indexToSortMode(idx: number): string {
  switch (idx) {
    case 0:
      return 'ByAddTime';
    case 1:
      return 'ByName';
    case 2:
      return 'ByPlayTimes';
    default:
      return 'ByAddTime';
  }
}

const isAscending = ref(true); // true = 升序，与 C++ ascendingWay 默认值一致

const isImporting = ref(false); // 控制按钮禁用状态和加载动画

async function handleFilesSelected(): Promise<void> {
  if (isImporting.value) return;
  isImporting.value = true;
  try {
    const results = await callJuceFunc(BRIDGE_inputFiles.name);
  } catch (error) {
    console.error(error);
  }
}

function toggleMyLike() {
  isAscending.value = !isAscending.value;
  localStorage.setItem('AllMusic_isascending', String(isAscending.value));
}

onMounted(() => {
  isAscending.value = localStorage.getItem('AllMusic_isascending') !== 'false'; //默认为升序(true)
});
</script>

<template>
  <div class="all-music-page">
    <!-- ================================================================
         第 1 行：
         allMusicLabel + numSongsLabel
         ================================================================ -->
    <div class="header-row">
      <!-- BigLabel: "全部音乐" — fontSize 30，与 C++ BigLabel 一致 -->
      <span class="page-title">所有音乐</span>

      <!-- littleLabel: "共 0 首" — fontSize 15，与 C++ littleLabel 一致 -->
      <span class="song-count">共 {{ songCount }} 首</span>
    </div>

    <!-- ================================================================
         第 2 行：工具栏行
         ================================================================ -->
    <div class="toolbar-row">
      <!--
        "导入文件" 按钮 — 对应 C++ yTextButton selectFileButton
        宽度 80px，与 C++ row2.removeFromLeft(80) 一致
      -->
      <div class="left-column">
        <button class="btn-import" :disabled="isImporting" @click="handleFilesSelected">
          导入文件/扫描文件夹
        </button>
      </div>

      <div class="right-column">
        <select v-model="selectedSort" class="combo-sort" title="排序方式">
          <!-- option本身不影响鼠标悬停事件 -->
          <option v-for="opt in sortOptions" :key="opt.value" :value="opt.value">
            {{ opt.label }}
          </option>
        </select>

        <button class="svg-button" @click="toggleMyLike()">
          <!-- 升降序标签 -->
          <IconArrowBigUpFilled v-if="isAscending" title="升序"></IconArrowBigUpFilled>
          <IconArrowBigDownFilled v-else title="降序"></IconArrowBigDownFilled>
        </button>
      </div>
    </div>

    <div class="viewport-area">
      <div class="viewport-placeholder"></div>
    </div>
  </div>
</template>

<style scoped>
.all-music-page {
  display: flex;
  flex-direction: column;
  /* 自上而下排列 */

  background-color: var(--color-main);
  height: 100%;
  width: 100%;
}

/* ================================================================
   第 1 行：标题行 
   ================================================================ */

.header-row {
  display: flex;
  align-items: center;
  gap: 15px;
  height: 50px;
  width: 100%;
}

.page-title {
  font-size: var(--big-font);
  font-weight: bold;
  color: var(--color-text-main);
  user-select: none;
}

.song-count {
  font-size: var(--little-font);
  color: var(--color-text-second);
  min-width: 90px;
}
/* ================================================================
   第 2 行：工具栏行 — 对应 C++ row2
   ================================================================ */

.toolbar-row {
  display: flex;
  align-items: center;
  /* 让所有子元素在垂直方向（交叉轴）上居中对齐 */
  gap: 6px;
  height: 80px;
  flex-shrink: 0;
  justify-content: space-between; /* 两端对齐：左侧靠左，右侧靠右 */
  padding: 10px 0;
}

.left-column {
  margin-right: auto;
  padding: 5px;
  /* 这部分里面的靠左对齐，其他靠右对齐 */
}

/* ── "导入文件" 按钮*/
.btn-import {
  min-width: 180px;
  height: 40px;
  margin: 10px;

  border: none;
  border-radius: var(--border-radius);
  background-color: var(--color-clicked);
  color: var(--color-text-main);
  font-size: var(--mid-font);
  cursor: pointer;
  transition:
    background-color var(--ease-time) ease,
    opacity var(--ease-time) ease;
}

.btn-import:hover {
  background-color: var(--color-hover);
}

.btn-import:active {
  background-color: var(--color-hover);
}

.btn-import:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.right-column {
  padding: 10px;
  justify-content: center;
  align-items: center;
  gap: 8px;
  height: 100%;
}

.combo-sort {
  width: 200px;
  height: 100%;
  margin: 0 20px;

  border: 3px solid var(--color-edge);
  border-radius: var(--border-radius);
  background-color: transparent;
  color: var(--color-text-main);
  font-size: var(--mid-font);
  cursor: pointer;
  outline: none;

  transition: border-color var(--ease-time) ease;
}

.combo-sort:hover {
  border-color: var(--color-text-second);
}

.combo-sort:focus {
  border-color: var(--color-stress);
}

/* ================================================================
   第 3 块：Viewport 区域 — 对应 C++ mViewPort
   占满剩余高度（约 80%）
   ================================================================ */

.viewport-area {
  flex: 1;
  overflow-y: auto;
  border-top: 2px solid var(--color-edge);
  position: relative;
}

.viewport-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  gap: 8px;
}

.placeholder-text {
  font-size: var(--big-font);
  color: var(--color-text-second);
}

.placeholder-hint {
  font-size: var(--little-font);
  color: var(--color-edge);
}
</style>
