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
    /** 当前页码第一首歌曲对应的全局序号 */
    beginSongIndex: 0,
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

    /**
     * 根据 songId 计算全局展示序号。
     * 公式：beginSongIndex + 在 songs 数组中的位置 + 1
     * 找不到返回 0。
     */
    getSongIndexById: (state) => {
      return (songId: number): number => {
        const idx = state.songs.findIndex((s) => s.songId === songId)
        return idx === -1 ? 0 : state.beginSongIndex + idx + 1
      }
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
      this.beginSongIndex = beginIndex
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

// ════════════════════════════════════════════════════════════════
// getSongMetadataLines — 构建歌曲元数据文本行
//
// 与 C++ OtherSongInfoIntro 的逻辑一致。
// 从 songInfo 中提取可展示的字段，跳过 undefined / 空 / 无效值。
// 作为纯函数放在 store 模块中，eachSong 和弹出窗内容组件共享。
// ════════════════════════════════════════════════════════════════

export function getSongMetadataLines(info: SongInfo): string[] {
  const lines: string[] = []

  const addIf = (label: string, value: string | number | undefined): void => {
    if (value === undefined) return
    const sv = String(value)
    if (sv !== '' && sv !== '0' && sv !== '-1') {
      lines.push(`${label}: ${sv}`)
    }
  }

  addIf('BPM', info.bpm)
  addIf('调性', info.key)
  if (info.sampleRate > 0) addIf('采样率', `${info.sampleRate} Hz`)
  if (info.bitRate > 0) addIf('比特率', `${info.bitRate} kbps`)
  addIf('通道数', info.numChannels)
  addIf('位深', info.bitDepth)
  addIf('解码器名称', info.codecName)
  lines.push(`是否已经进行过AI分析: ${info.aiProcessed ? '是' : '否'}`)
  addIf('AI分析体裁', info.aiGenre)
  lines.push(`是否为音乐资源: ${info.isMusic ? '是' : '否'}`)
  addIf('专辑艺术家', info.albumArtist)
  addIf('体裁', info.genre)
  if (info.trackNumber !== undefined && info.trackNumber >= 0) addIf('轨道号', info.trackNumber)
  if (info.discNumber !== undefined && info.discNumber > 0) addIf('碟片号', info.discNumber)
  addIf('发行年份', info.year)
  addIf('作曲者', info.composer)
  addIf('文件路径', info.filePath)
  addIf('文件大小', info.fileSize)
  addIf('最后修改时间', info.lastModifiedTime)

  return lines
}
