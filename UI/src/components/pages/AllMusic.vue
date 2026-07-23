<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';

import { B_getAllSongCount, B_songImport, B_songInfo } from '@/bridge/bridge.generated.ts';
import { IconArrowBigDownFilled, IconArrowBigUpFilled } from '@tabler/icons-vue';
import { useSongStore, type SongInfo } from '@/store/songStore';
import { useVirtualizer } from '@tanstack/vue-virtual';
import EachSong from '../cell/eachSong.vue';
import { getNativeFunction } from 'juce-framework-frontend-mirror';
import { showInfoWindow } from '../other/infoWindow.vue';

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

async function songImport() {
  if (isImporting.value) return;
  isImporting.value = true;
  try {
    const obj = await getNativeFunction(B_songImport.name)();
    console.log('songImport result:', obj, typeof obj);
    if (obj && typeof obj === 'object') {
      const numImport = obj[B_songImport.numImport];
      const numSuccess = obj[B_songImport.numSuccess];
      songStore.songs.push(...obj[B_songImport.songs]);
      //...的意思是解包，防止直接推入一整个数组放在尾部
      let windowText: string =
        '导入歌曲完成\n总共导入' + numImport + '个文件' + '\n成功' + numSuccess + '个文件';

      const errorFiles = obj[B_songImport.errorFiles] as string[];
      const errorFilesLength = errorFiles.length;

      if (errorFilesLength !== 0) {
        windowText += '\n失败文件:\n';
        for (let i = 0; i < errorFilesLength; i++) {
          windowText += errorFiles[i] + '\n';
        }
      }
      showInfoWindow(windowText);
    }
  } catch (error) {
    console.error(error);
  } finally {
    await refreshSongCount();
    isImporting.value = false;
  }
}
async function refreshSongCount() {
  const obj = await getNativeFunction(B_getAllSongCount.name)();
  if (obj && typeof obj === 'object' && B_getAllSongCount.count in obj) {
    songCount.value = obj[B_getAllSongCount.count];
  }
}
const songStore = useSongStore();

const scrollContainerRef = ref<HTMLElement | null>(null); //滚动容器窗口

const sortSongs = computed(() => {
  const copy = [...songStore.songs]; //浅拷贝(只拷贝指针)
  switch (selectedSort.value) {
    case 0:
      //添加时间排序
      if (isAscending.value) {
        return copy.sort((a, b) => a.songId - b.songId);
      } else {
        return copy.sort((a, b) => b.songId - a.songId);
      }
    case 1:
      //歌曲名称排序
      const collator = new Intl.Collator('en', { numeric: true });
      if (isAscending.value) {
        return copy.sort((a, b) => collator.compare(a.title, b.title));
      } else {
        return copy.sort((a, b) => collator.compare(b.title, a.title));
      }
    case 2:
      //播放次数排序
      if (isAscending.value) {
        return copy.sort((a, b) => a.playNum - b.playNum);
      } else {
        return copy.sort((a, b) => b.playNum - a.playNum);
      }
    default:
      return copy;
  }
});

const virtualizer = useVirtualizer(
  computed(() => ({
    count: sortSongs.value.length,
    getScrollElement: () => scrollContainerRef.value,
    // 每行固定高度
    estimateSize: () => 80,
    // 上下多预渲染 10 个节点，防止快速滚动白屏
    overscan: 10,
  })),
);

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
        <button class="btn-import" :disabled="isImporting" @click="songImport()">
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

    <div style="flex: 1; overflow-y: auto" ref="scrollContainerRef">
      <!-- 外层视口 -->
      <div :style="{ height: `${virtualizer.getTotalSize()}px` }" style="position: relative">
        <!-- 内层viewport ，因为v-bind需要传进来一个js对象，所以必须使用花括号-->
        <EachSong
          v-for="virtualRow in virtualizer.getVirtualItems()"
          :key="String(virtualRow.key)"
          :style="{
            transform: `translateY(${virtualRow.start}px)`,
            height: `${virtualRow.size}px`,
          }"
          style="position: absolute; top: 0; left: 0"
          :song="sortSongs[virtualRow.index]!"
        >
          <!-- 虚拟滚动情景使用transform对GPU更友好 -->
        </EachSong>
      </div>
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
</style>
