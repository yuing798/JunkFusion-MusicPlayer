// stores/playBackStore.ts
import { defineStore } from 'pinia';

export const usePlayBackStore = defineStore('playBack', {
  state: () => ({
    /** 当前正在播放的歌曲 ID，null 表示未播放任何歌曲 */
    currentSongId: null as number | null,
    /** 是否正在播放（true 表示播放中，false 表示暂停） */
    isPlaying: false,
  }),

  actions: {
    /**
     * 播放指定歌曲。
     * 如果传入的 songId 与当前播放 ID 相同，则只将 isPlaying 设为 true（继续播放）。
     * 否则切换歌曲并开始播放。
     */
    play(songId: number) {
      // 如果切换到新歌曲，则更新 currentSongId
      if (this.currentSongId !== songId) {
        this.currentSongId = songId;
      }
      this.isPlaying = true;
      // 此处可调用实际的播放器 API（如 HTML5 Audio 或原生插件）
    },

    /** 暂停当前播放 */
    pause() {
      this.isPlaying = false;
      // 可调用实际播放器暂停方法
    },

    /** 切换播放/暂停状态 */
    // togglePlay() {
    //   if (this.isPlaying) {
    //     this.pause();
    //   } else {
    //     // 如果当前没有歌曲，则默认播放第一首（可选）
    //     if (this.currentSongId === null) {
    //       const songStore = useSongStore();
    //       if (songStore.songs.length > 0) {
    //         this.play(songStore.songs[0].songId);
    //       }
    //       return;
    //     }
    //     // 否则继续播放当前歌曲
    //     this.isPlaying = true;
    //     // 可调用实际播放器 resume 方法
    //   }
    // },

    /** 重置播放状态（例如切换页面时清空播放） */
    stop() {
      this.currentSongId = null;
      this.isPlaying = false;
    },
  },
});