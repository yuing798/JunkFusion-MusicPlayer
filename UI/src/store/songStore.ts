import { defineStore } from 'pinia'
import { callJuceFunc } from '@/bridge/bridgeSupport'
import { showErrorWindow } from '@/components/other/errorWindow.vue'
import { B_toggleMyLike } from '@/bridge/bridge.generated'

// TypeScript 的 interface：它只在编译时存在，用来检查类型。编译成 JavaScript 后，它会被完全删除，不留任何痕迹。
export interface SongInfo {//interface指的是自定义类型

  /** 数据库主键，自增 ID，C++ 端为 int64_t */
  songId: number

  /** 歌曲时长（秒） */
  duration: number

  title: string

  /** 艺术家名称，C++ 端为 std::optional */
  artist: string|null

  /** 专辑名称，C++ 端为 std::optional */
  album: string|null

  /** 专辑艺术家，C++ 端为 std::optional */
  albumArtist: string|null

  /** 体裁，C++ 端为 std::optional */
  genre: string|null

  /** 轨道号，C++ 端为 std::optional（undefined 表示不存在） */
  trackNumber: number|null

  /** 碟片号，C++ 端为 std::optional（undefined 表示不存在） */
  discNumber: number|null

  /** 发行年份，C++ 端为 std::optional（undefined 表示不存在） */
  year: number|null

  /** 作曲者，C++ 端为 std::optional */
  composer: string|null

  /** 比特率（kbps），C++ 端为 int64_t */
  bitRate: number

  /** 采样率（Hz） */
  sampleRate: number

  /** 通道数 */
  numChannels: number

  /** 位深 */
  bitDepth: number

  /** 编码器名称，C++ 端为 std::optional */
  codecName: string|null

  /** 是否被检测为音乐资源 */
  isMusic: boolean

  /** AI 分析体裁，C++ 端为 std::optional */
  aiGenre: string|null

  /** 节拍数（BPM），C++ 端为 std::optional（undefined 表示未知） */
  bpm: number|null

  /** 调性（如 "C major", "A minor"），C++ 端为 std::optional */
  key: string|null

  // ════════════════════════════════════════════════════════════════
  // 5. 用户信息
  // ════════════════════════════════════════════════════════════════

  /** 是否添加到"我喜欢"列表 */
  isMyLike: boolean

  /** 用户备注，C++ 端为 std::optional */
  comment: string|null

  /** 已经播放了多少次 */
  playNum: number

}
// ════════════════════════════════════════════════════════════════
// songStore — 当前页面歌曲列表的 Pinia store
//
// 职责：
//   - 持有当前页歌曲数据（songs），避免通过 props 层层传递 SongInfo
//   - 提供 getSongById 让子组件按 ID 查找
//   - 统一管理"我喜欢"的乐观更新与回滚
// ════════════════════════════════════════════════════════════════

export const useSongStore = defineStore('songPage', {
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
    setPageData(songs: SongInfo[]) {
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
        await callJuceFunc(B_toggleMyLike.name, songId)
      } catch (error) {
        showErrorWindow(error)
        this.songs[index] = originSong // 回滚
      }
    },
  },
})