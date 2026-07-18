<script setup lang="ts">
import { onMounted, ref, watch } from 'vue';

import { callJuceFunc } from '@/bridge/bridgeSupport.ts';
import {
  B_getAllSongCount,
  B_inputFiles,
  B_refreshAllMusicSongs,
  B_songInfo,
} from '@/bridge/bridge.generated.ts';
import { IconArrowBigDownFilled, IconArrowBigUpFilled } from '@tabler/icons-vue';
import { useSongStore, type SongInfo } from '@/store/songStore';

const songCount = ref(0);

const sortOptions = [
  { value: 0, label: '添加时间' }, // SortMode::ByAddTime
  { value: 1, label: '歌曲名称' }, // SortMode::ByName
  { value: 2, label: '播放次数' }, // SortMode::ByPlayTimes
];
const selectedSort = ref(0);

watch(selectedSort, (value) => {
  localStorage.setItem('AllMusic_sortMode', String(value));
});

const isAscending = ref(true); // true = 升序

const isImporting = ref(false); // 控制按钮禁用状态和加载动画

function toggleAscending() {
  isAscending.value = !isAscending.value;
  localStorage.setItem('AllMusic_isascending', String(isAscending.value));
}

async function handleFilesSelected(): Promise<void> {
  if (isImporting.value) return;
  isImporting.value = true;
  try {
    await callJuceFunc(B_inputFiles.name);
  } catch (error) {
    console.error(error);
  } finally {
    await refreshSongCount();
    isImporting.value = false;
  }
}
async function refreshSongCount() {
  const obj = await callJuceFunc(B_getAllSongCount.name);
  songCount.value = (obj as any)?.[B_getAllSongCount.count];
}
const songStore = useSongStore();

async function refreshSongPage(targetPage: number, isAscending: boolean, sortMode: number) {
  const result = await callJuceFunc(B_refreshAllMusicSongs.name, {
    page: targetPage,
    isAscending: isAscending,
    sortMode: sortMode,
  });
  if (result) {
    if (typeof result === 'object' && B_refreshAllMusicSongs.error in result) {
      refreshErrorPage(); //错误页面刷新出来需要图片占位
      return;
    }
    if (Array.isArray(result)) {
      const pageList = result as SongInfo[];
      songStore.setPageData(pageList);
      return;
    }
  }
}

function refreshErrorPage() {
  //错误页面显示图像
}

onMounted(() => {
  //初始化升降序
  isAscending.value = localStorage.getItem('AllMusic_isascending') !== 'false'; //默认为升序(true)

  //初始化排序方案
  const sortWaysLoad = localStorage.getItem('AllMusic_sortMode');
  if (sortWaysLoad !== null) {
    selectedSort.value = Number(sortWaysLoad);
  } else {
    selectedSort.value = 0;
  }

  refreshSongCount(); //获得歌曲总数
});
// computed的几个特性
// 必须有返回值（它“计算”出结果）。
// 依赖其他响应式数据（依赖变了，它自动重新计算）。
// 必须是同步的（不能在里面写 setTimeout 或 await）。
// 不应该产生“副作用”（即不应该修改其他数据、不应该操作 DOM、不应该读写 localStorage）。
//computed 的典型场景是：“前端手里已经有一份完整数据，我需要基于它算出一个新值”。
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

        <button class="svg-button" @click="toggleAscending()">
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
