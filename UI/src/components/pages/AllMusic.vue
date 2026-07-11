<script setup lang="ts">
import { onMounted, ref } from 'vue';

import { callJuceFunc } from '@/bridge/bridgeSupport.ts';
import { BRIDGE_inputFiles } from '@/bridge/bridge.generated.ts';
import { IconArrowBigDownFilled, IconArrowBigUpFilled } from '@tabler/icons-vue';

const songCount = ref(0);

const sortOptions = [
  { value: 1, label: '添加时间排列' }, // SortMode::ByAddTime
  { value: 0, label: '歌曲名称排列' }, // SortMode::ByName
  { value: 2, label: '播放次数排列' }, // SortMode::ByPlayTimes
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
      <span class="page-title">全部音乐</span>

      <!-- littleLabel: "共 0 首" — fontSize 15，与 C++ littleLabel 一致 -->
      <span class="song-count">共 {{ songCount }} 首</span>
    </div>

    <!-- ================================================================
         第 2 行：工具栏行（对应 C++ resized() 中的 row2）
         selectFileButton / refreshButton / seqWays combo / upDownButton
         ================================================================ -->
    <div class="toolbar-row">
      <!--
        "导入文件" 按钮 — 对应 C++ yTextButton selectFileButton
        宽度 80px，与 C++ row2.removeFromLeft(80) 一致
      -->
      <button class="btn-import" :disabled="isImporting" @click="handleFilesSelected">
        导入音频文件
      </button>

      <div class="toolbar-spacer"></div>

      <select v-model="selectedSort" class="combo-sort">
        <option v-for="opt in sortOptions" :key="opt.value" :value="opt.value">
          {{ opt.label }}
        </option>
      </select>

      <button class="svg-button" @click="toggleMyLike()">
        <IconArrowBigUpFilled v-if="isAscending" title="升序"></IconArrowBigUpFilled>
        <IconArrowBigDownFilled v-else title="降序"></IconArrowBigDownFilled>
      </button>
    </div>

    <!-- ================================================================
         第 3 块：歌曲列表区域（对应 C++ mViewPort）
         此处是占位区域，后续由 SongSelectViewport 可复用组件替代
         ================================================================ -->
    <div class="viewport-area">
      <!--
        TODO: 放置 <SongSelectViewport /> 组件
        对应 C++ 中的 mSongSelectViewport（SongSelectViewport 类型）
      -->
      <div class="viewport-placeholder">
        <span class="placeholder-text">歌曲列表区域</span>
        <span class="placeholder-hint">（SongSelectViewport 组件待开发）</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
.all-music-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  background-color: var(--color-main);
  user-select: none;
}

/* ================================================================
   第 1 行：标题行 — 对应 C++ row1
   ================================================================ */

.header-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 5px;
  flex-shrink: 0;
}

/* BigLabel: "全部音乐" — fontSize 30（与 C++ BigLabel 一致） */
.page-title {
  font-size: 30px;
  font-weight: bold;
  color: var(--color-text-main);
  min-width: 90px; /* 对应 row1.removeFromLeft(90) */
}

/* littleLabel: "共 0 首" — fontSize 15（与 C++ littleLabel 一致） */
.song-count {
  font-size: var(--little-font);
  color: var(--color-text-main);
  min-width: 90px; /* 对应 row1.removeFromLeft(90) */
}

/* ================================================================
   第 2 行：工具栏行 — 对应 C++ row2
   removeFromLeft(20) → margin-left
   每个控件 reduced(5) → 内缩
   ================================================================ */

.toolbar-row {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 5px;
  padding-left: 20px; /* 对应 row2.removeFromLeft(20) */
  padding-right: 40px; /* 对应 row2.removeFromRight(40) */
  flex-shrink: 0;
}

/* ── "导入文件" 按钮：对应 yTextButton selectFileButton ── */
.btn-import {
  min-width: 80px; /* 对应 row2.removeFromLeft(80) */
  height: 30px; /* 对应 reduced(5,10)：40 - 10 = 30 */
  padding: 0 12px;

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

/* ── 弹性空间：将排序控件推到右侧 ── */
.toolbar-spacer {
  flex: 1;
}

/* ── 排序下拉框：对应 YComboBox seqWays ── */
.combo-sort {
  width: 200px; /* 对应 row2.removeFromRight(200) */
  height: 30px;
  padding: 0 8px;

  border: 3px solid var(--color-edge); /* 与 YComboBox kOutlineWidth = 3 一致 */
  border-radius: var(--border-radius); /* 与 YComboBox kCornerRadius = 6 一致 */
  background-color: transparent;
  color: var(--color-text-main);
  font-size: 17px; /* 与 YComboBox kFontSize = 17 一致 */
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
