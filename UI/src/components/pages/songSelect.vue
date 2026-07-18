<script setup lang="ts">
import { watch } from 'vue';
import EachSong from '@/components/cell/eachSong.vue';
import PageChange from '@/components/other/pageChange.vue';
import { useSongStore } from '@/store/songStore';

//该处为分页加载

// ════════════════════════════════════════════════════════════════
// SongSelectViewport — 歌曲列表滚动容器
//
// 数据流向：
//   1. 父组件传入 songInfos（当前页歌曲数据）
//   2. songSelect 调用 songStore.setPageData() 写入 store
//   3. eachSong 通过 songId 从 store 查找完整 SongInfo —— 无 props 传递
//   4. toggle-like 直接在 store 内完成（乐观更新），不 emit
//   5. 更多信息弹窗在 eachSong 内部（PopupWindow 包裹），不 emit
//   6. 只有 request-playback-change 需要冒泡到上层
//
// 使用方式：
//   <SongSelectViewport
//     :totalPages="10"
//     :currentPage="1"
//     :songInfos="songs"
//     :activeSongId="playingSongId"
//     :activePlaybackState="currentPlaybackState"
//     @page-change="handlePageChange"
//     @request-playback-change="handlePlaybackChange"
//   />
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
// 每首歌曲的播放状态
// 只有当 song.songId === activeSongId 时才显示 activePlaybackState，
// 否则为 Stopped（显示序号）
// ════════════════════════════════════════════════════════════════

const songStore = useSongStore();

function onPageChange(page: number): void {
  emit('page-change', page);
}
</script>

<template>
  <!--
    SongSelectViewport — 歌曲列表滚动容器
    对应 C++ SongSelectViewport 类
    布局：N 首歌曲行（每行 70px）+ PageChange 组件（紧跟最后一首歌曲）
    最多预留 15 个槽位
  -->
  <div class="song-select-viewport">
    <!-- ═══════════════════════════════════════════════════════════
         歌曲行列表
         每行 70px，最多 15 行
         只传 songId + playbackState：
           - songId     → eachSong 从 store 查找 SongInfo 和计算序号
           - playbackState → 控制播放/暂停图标
         ═══════════════════════════════════════════════════════════ -->
    <div class="song-list">
      <EachSong v-for="song in songStore.songs" :key="song.songId" :song-id="song.songId" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         PageChange — 页面切换组件
         紧跟最后一首歌曲下方，不固定在底部
         没有歌曲时不显示（totalPages <= 0）
         ═══════════════════════════════════════════════════════════ -->
    <PageChange
      v-if="totalPages > 0"
      :total-pages="totalPages"
      :current-page="currentPage"
      class="page-change-area"
      @page-change="onPageChange"
    />
  </div>
</template>

<style scoped>
/* ════════════════════════════════════════════════════════════════
   SongSelectViewport 容器
   对应 C++ SongSelectViewport
   ════════════════════════════════════════════════════════════════ */

.song-select-viewport {
  display: flex;
  flex-direction: column;
  /*
    不设固定高度；由父组件（AllMusicPage）通过 flex: 1 + overflow-y 控制滚动。
    pageChange 始终紧跟最后一首歌曲下方。
  */
  width: 100%;
}

/* ════════════════════════════════════════════════════════════════
   歌曲列表
   ════════════════════════════════════════════════════════════════ */

.song-list {
  display: flex;
  flex-direction: column;
  /* 每个 EachSong 行高 70px，由 eachSong 自身控制 */
}

/* ════════════════════════════════════════════════════════════════
   PageChange 区域
   居中显示，与 C++ 一致：
   mPageChange.setTopLeftPosition((getWidth()-mPageChange.getWidth())/2.0f, 0)
   ════════════════════════════════════════════════════════════════ */

.page-change-area {
  align-self: center;
  margin-top: 4px;
}
</style>
