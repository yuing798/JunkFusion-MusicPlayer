<script setup lang="ts">
import EachSong from '@/cell/eachSong.vue'
import PageChange from '@/other/pageChange.vue'
import { PlaybackState } from '@/UtilsScripts/playState'
import type { SongInfo } from '@/cell/SongInfo'

// ════════════════════════════════════════════════════════════════
// SongSelectViewport — 歌曲列表滚动容器
//
// 使用方式：
//   <SongSelectViewport
//     :totalPages="10"
//     :currentPage="1"
//     :songInfos="songs"
//     :activeSongId="playingSongId"
//     :activePlaybackState="currentPlaybackState"
//     @page-change="handlePageChange"
//     @toggle-like="handleToggleLike"
//     @show-more-info="handleShowMoreInfo"
//     @request-playback-change="handlePlaybackChange"
//   />
//
// Props：
//   totalPages          — 总页码数
//   currentPage         — 当前选中的页码（1 起步）
//   songInfos           — 当前页的歌曲信息数组（最多 15 首）
//   activeSongId        — 当前正在播放的歌曲 ID，null 表示无歌曲播放
//   activePlaybackState — 当前播放状态（Playing/Paused/Stopped）
//
// Emits：
//   page-change            — 用户点击了某个页码
//   toggle-like            — 用户切换喜欢状态（从 eachSong 冒泡）
//   show-more-info         — 用户点击更多信息（从 eachSong 冒泡）
//   request-playback-change — 用户点击序号/播放图标（从 eachSong 冒泡）
// ════════════════════════════════════════════════════════════════

const props = defineProps<{
  /** 总页码数 */
  totalPages: number
  /** 当前选中的页码（从 1 开始） */
  currentPage: number
  /** 当前页的歌曲信息数组，最多 15 首 */
  songInfos: SongInfo[]
  /** 当前正在播放的歌曲 ID，null 表示无歌曲播放 */
  activeSongId: number | null
  /** 当前播放状态 */
  activePlaybackState: PlaybackState
}>()

const emit = defineEmits<{
  (e: 'page-change', page: number): void
  (e: 'toggle-like', info: SongInfo): void
  (e: 'show-more-info', info: SongInfo): void
  (e: 'request-playback-change', info: SongInfo, nextState: PlaybackState): void
}>()

// ════════════════════════════════════════════════════════════════
// 每首歌曲的播放状态
// 只有当 song.songId === activeSongId 时才显示 activePlaybackState，
// 否则为 Stopped（显示序号）
// ════════════════════════════════════════════════════════════════

function getSongPlaybackState(song: SongInfo): PlaybackState {
  if (props.activeSongId !== null && song.songId === props.activeSongId) {
    return props.activePlaybackState
  }
  return PlaybackState.Stopped
}

// ════════════════════════════════════════════════════════════════
// 事件冒泡：将 eachSong 和 pageChange 的事件向上传递
// ════════════════════════════════════════════════════════════════

function onToggleLike(info: SongInfo): void {
  emit('toggle-like', info)
}

function onShowMoreInfo(info: SongInfo): void {
  emit('show-more-info', info)
}

function onRequestPlaybackChange(info: SongInfo, nextState: PlaybackState): void {
  emit('request-playback-change', info, nextState)
}

function onPageChange(page: number): void {
  emit('page-change', page)
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
         ═══════════════════════════════════════════════════════════ -->
    <div class="song-list">
      <EachSong
        v-for="(song, index) in songInfos"
        :key="song.songId"
        :song-index="(currentPage - 1) * 15 + index + 1"
        :song-info="song"
        :playback-state="getSongPlaybackState(song)"
        @toggle-like="onToggleLike"
        @show-more-info="onShowMoreInfo"
        @request-playback-change="onRequestPlaybackChange"
      />
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
