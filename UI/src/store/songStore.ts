import { defineStore } from 'pinia'
import type { SongInfo } from './SongInfo'
import { callJuceFunc } from '@/bridge/bridgeSupport'
import { BRIDGE_KEYS } from '@/bridge/bridge.generated'
import { showErrorWindow } from '@/components/other/errorWindow.vue'

// ════════════════════════════════════════════════════════════════
// songStore — 当前页面歌曲列表的 Pinia store
// ════════════════════════════════════════════════════════════════

export const songStore = defineStore('songPage', {
  state: () => ({
    songs: [] as SongInfo[], // 当前展示的歌曲信息
  }),

  actions: {
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
        await callJuceFunc<void>(BRIDGE_KEYS.toggleMyLike, songId)
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
// 作为纯函数放在 store 模块中，所有消费方（eachSong、弹出窗等）共享同一份逻辑。
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
