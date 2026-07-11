import { defineStore } from 'pinia'
import type { SongInfo } from './SongInfo'
import { callJuceFunc } from '@/bridge/bridgeSupport'
import { showErrorWindow } from '@/components/other/errorWindow.vue'
import { B_toggleMyLike } from '@/bridge/bridge.generated'

// ════════════════════════════════════════════════════════════════
// songStore — 当前页面歌曲列表的 Pinia store
//
// 职责：
//   - 持有当前页歌曲数据（songs），避免通过 props 层层传递 SongInfo
//   - 提供 getSongById 让子组件按 ID 查找
//   - 统一管理"我喜欢"的乐观更新与回滚
// ════════════════════════════════════════════════════════════════

export const songStore = defineStore('songPage', {
  state: () => ({
    /** 当前展示的歌曲信息（一页最多 15 首） */
    songs: [] as SongInfo[],
  }),

  getters: {
    /**
     * 根据 songId 查找歌曲。
     * 找不到返回 undefined。
     *
     * 用法（组件中）：
     *   const song = mySongStore.getSongById(props.songId)
     */
    getSongById: (state) => {
      return (songId: number): SongInfo | undefined =>
        state.songs.find((s) => s.songId === songId)
    },

  },

  actions: {
    /**
     * 设置当前页的歌曲数据。
     * 由父组件（songSelect / AllMusic）在拿到新页面数据后调用。
     *
     * @param songs       - 当前页歌曲数组
     * @param beginIndex  - 当前页第一首歌曲的全局序号（用于计算每行的 displayNumber）
     */
    setPageData(songs: SongInfo[], beginIndex: number) {
      this.songs = songs
    },

    /** 切换"我喜欢"状态（乐观更新 + 回滚） */
    async toggleMyLike(songId: number) {
      const index = this.songs.findIndex((s) => s.songId === songId)
      if (index === -1) return
      const originSong = this.songs[index]!
      // 乐观更新
      this.songs[index] = {
        ...originSong,
        isMyLike: !originSong.isMyLike,
      }
      try {
        await callJuceFunc<void>(B_toggleMyLike.name, songId)
      } catch (error) {
        showErrorWindow(error)
        this.songs[index] = originSong // 回滚
      }
    },
  },
})