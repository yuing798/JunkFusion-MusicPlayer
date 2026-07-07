<script setup lang="ts">
import { ref, computed, nextTick, onUnmounted } from 'vue'
import Tooltip from '@/other/tooltip.vue'
import { PlaybackState } from '@/UtilsScripts/playState'
import type { SongInfo } from '@/cell/SongInfo'

// ── SVG 资源 ──
import playSvg from '@/assets/image/play.svg'
import pauseSvg from '@/assets/image/pause.svg'
import heartSvg from '@/assets/image/heart.svg'
import heartFillSvg from '@/assets/image/heart-fill.svg'
import whatsMoreSvg from '@/assets/image/whatsMore.svg'
import xSvg from '@/assets/image/x.svg'

// ════════════════════════════════════════════════════════════════
// Props & Emits
// ════════════════════════════════════════════════════════════════

const props = defineProps<{
  //父到子
  /** 歌曲全局序号 */
  songIndex: number
  /** 歌曲完整信息 */
  songInfo: SongInfo
  /** 当前播放状态 */
  playbackState: PlaybackState
}>()

const emit = defineEmits<{
  (e: 'toggle-like', info: SongInfo): void
  (e: 'show-more-info', info: SongInfo): void
  /** 请求改变播放状态：Stopped → Playing, Playing → Paused, Paused → Playing */
  (e: 'request-playback-change', info: SongInfo, nextState: PlaybackState): void
}>()

/** 多流音频悬停提示文本 */
const MULTI_STREAM_TOOLTIP_TEXT =
  '该文件包含多路音频流（如多语言、多声道）。当前播放器将自动为您选择质量最佳的默认音轨。如需切换其他音轨，请使用专业音频工具（如 MKVToolNix）自行调整文件封装顺序'

// ════════════════════════════════════════════════════════════════
// 弹出窗状态
// ════════════════════════════════════════════════════════════════

const moreButtonRef = ref<HTMLElement | null>(null)
const popupRef = ref<HTMLElement | null>(null)
const popupVisible = ref(false)
const popupClosing = ref(false)
const popupPosition = ref({ left: '0px', top: '0px' })

// ── 拖动状态 ──
const isDragging = ref(false)
const dragOffset = ref({ x: 0, y: 0 })

/** 弹出窗更多信息按钮点击 */
function handleMoreClick(): void {
  if (popupVisible.value) {
    closePopup()
    return
  }

  // 捕获按钮中心坐标（viewport 坐标系），作为动画变换原点
  if (moreButtonRef.value) {
    const rect = moreButtonRef.value.getBoundingClientRect()
    document.documentElement.style.setProperty(
      '--popup-origin-x',
      rect.left + rect.width / 2 + 'px',
    )
    document.documentElement.style.setProperty(
      '--popup-origin-y',
      rect.top + rect.height / 2 + 'px',
    )
  } //动态改变css属性

  popupVisible.value = true
  popupClosing.value = false

  // 初始居中
  popupPosition.value = {
    left: (window.innerWidth - 400) / 2 + 'px',
    top: (window.innerHeight - 350) / 2 + 'px',
  }

  // 用实际尺寸重新居中
  void nextTick(() => {
    if (popupRef.value) {
      const r = popupRef.value.getBoundingClientRect()
      popupPosition.value = {
        left: (window.innerWidth - r.width) / 2 + 'px',
        top: (window.innerHeight - r.height) / 2 + 'px',
      }
    }
  })

  emit('show-more-info', props.songInfo)
}

function closePopup(): void {
  popupClosing.value = true
  setTimeout(() => {
    popupVisible.value = false
    popupClosing.value = false
  }, 200) // 匹配动画时长
}

// ── 拖动 ──

function startDrag(e: MouseEvent): void {
  // 点击关闭按钮时不触发拖动
  if ((e.target as HTMLElement).closest('.popup-close-btn')) return

  isDragging.value = true
  const rect = popupRef.value!.getBoundingClientRect()
  dragOffset.value = { x: e.clientX - rect.left, y: e.clientY - rect.top }
  // 计算鼠标在弹窗内部的位置

  window.addEventListener('mousemove', onDragMove) //'mousemove'是浏览器提供的API方法
  window.addEventListener('mouseup', onDragEnd) //用户在任意位置松开鼠标左键（或触控板左键）的那一刻
}

function onDragMove(e: MouseEvent): void {
  if (!isDragging.value) return
  popupPosition.value = {
    left: e.clientX - dragOffset.value.x + 'px',
    top: e.clientY - dragOffset.value.y + 'px',
  }
}

function onDragEnd(): void {
  isDragging.value = false
  window.removeEventListener('mousemove', onDragMove)
  window.removeEventListener('mouseup', onDragEnd)
}

// ════════════════════════════════════════════════════════════════
// 元数据文本构建（弹出窗内容，与 C++ OtherSongInfoIntro 一致）
// ════════════════════════════════════════════════════════════════

const metadataLines = computed(() => {
  const info = props.songInfo
  const lines: string[] = []

  const addIf = (label: string, value: string | number): void => {
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
  if (info.trackNumber >= 0) addIf('轨道号', info.trackNumber)
  if (info.discNumber > 0) addIf('碟片号', info.discNumber)
  addIf('发行年份', info.year)
  addIf('作曲者', info.composer)
  addIf('文件路径', info.filePath)
  addIf('文件大小', info.fileSize)
  addIf('最后修改时间', info.lastModifiedTime)

  return lines
})

// ════════════════════════════════════════════════════════════════
// 事件处理
// ════════════════════════════════════════════════════════════════

function handleLikeClick(): void {
  emit('toggle-like', props.songInfo)
}

/**
 * 点击序号/播放状态列的切换逻辑：
 *   Stopped → 请求进入 Playing
 *   Playing → 请求进入 Paused
 *   Paused  → 请求进入 Playing
 */
function handleOrdinalClick(): void {
  let nextState: PlaybackState
  switch (props.playbackState) {
    case PlaybackState.Stopped:
      nextState = PlaybackState.Playing
      break
    case PlaybackState.Playing:
      nextState = PlaybackState.Paused
      break
    case PlaybackState.Paused:
      nextState = PlaybackState.Playing
      break
  }
  emit('request-playback-change', props.songInfo, nextState)
}

// ════════════════════════════════════════════════════════════════
// 清理
// ════════════════════════════════════════════════════════════════

onUnmounted(() => {
  document.documentElement.style.removeProperty('--popup-origin-x')
  document.documentElement.style.removeProperty('--popup-origin-y')
  window.removeEventListener('mousemove', onDragMove)
  window.removeEventListener('mouseup', onDragEnd)
})
</script>

<template>
  <!--
    EachSong — 单首歌曲行
    对应 C++ 中的 EachSong 类
    7 列 Grid 布局：序号/状态 | 歌名+艺术家+多流 | 专辑 | AI分类 | 播放次数 | 喜欢 | 更多
  -->
  <div class="each-song-row">
    <!-- ═══════════════════════════════════════════════════════════
         第 1 列：序号 / 播放状态 (50px)
         Stopped → 显示序号数字 ->@click.stop:进入playing状态
         Playing → play.svg ->@click.stop:进入paused状态
         Paused → pause.svg ->@click.stop:进入playing状态
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-ordinal" @click.stop="handleOrdinalClick">
      <span v-if="playbackState === PlaybackState.Stopped" class="ordinal-number">
        {{ songIndex }}
      </span>
      <img
        v-else-if="playbackState === PlaybackState.Playing"
        :src="playSvg"
        class="state-icon"
        alt=""
      />
      <img v-else :src="pauseSvg" class="state-icon" alt="" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 2 列：歌名 + 艺术家 + 多流徽章 (flex: 1)
         垂直堆叠：上行 = 歌名 + 多流徽章，下行 = 艺术家名
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-name-artist">
      <div class="name-row">
        <Tooltip :text="songInfo.title">
          <span class="song-name">{{ songInfo.title }}</span>
          <!-- 歌曲是一定有标题的 -->
        </Tooltip>
        <Tooltip :text="MULTI_STREAM_TOOLTIP_TEXT">
          <span v-if="songInfo.isMultiStreamFile" class="multi-stream-badge"> 多流音频 </span>
        </Tooltip>
      </div>
      <div class="artist-row">
        <Tooltip :text="songInfo.artist || '未知'">
          <span class="artist-name">{{ songInfo.artist || '未知' }}</span>
        </Tooltip>
      </div>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 3 列：专辑名称 (150px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-album">
      <span class="ellipsis-text">{{ songInfo.album || '未知' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 4 列：AI 分类标签 (120px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-genre">
      <span class="ellipsis-text">{{ songInfo.aiGenre || '' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 5 列：播放次数 (80px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-play-count">
      <span class="play-count-text">{{ songInfo.hadPlayedNum }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 6 列：我喜欢按钮 (40px)
         isMyLike=true → heart-fill.svg（红心）
         isMyLike=false → heart.svg（空心）
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-like" @click.stop="handleLikeClick">
      <img :src="songInfo.isMyLike ? heartFillSvg : heartSvg" class="icon-btn" alt="like" />
      <!-- @click.stop用来阻止向父组件冒泡 -->
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 7 列：更多信息按钮 (40px)
         点击触发弹出窗，仿 C++ PopupWindowButton 的缩放动画
         ═══════════════════════════════════════════════════════════ -->
    <div ref="moreButtonRef" class="cell cell-more" @click.stop="handleMoreClick">
      <img :src="whatsMoreSvg" class="icon-btn" alt="更多信息" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         弹出窗 — Teleport to body
         仿 C++ popupWindow 的缩放仿射变换动画
         ═══════════════════════════════════════════════════════════ -->
    <Teleport to="body">
      <div
        v-if="popupVisible"
        ref="popupRef"
        class="popup-window"
        :class="{ 'popup-closing': popupClosing }"
        :style="popupPosition"
      >
        <!-- 可拖动标题栏 -->
        <div class="popup-titlebar" @mousedown="startDrag">
          <span class="popup-title">{{ songInfo.title || '未知' }}</span>
          <button class="popup-close-btn" @click="closePopup">
            <img :src="xSvg" alt="关闭" />
          </button>
        </div>

        <!-- 内容区域：专辑封面 + 元数据文本 -->
        <div class="popup-content">
          <!-- 专辑封面：后续通过 imageHash 桥接获取 -->
          <div class="popup-album-art-placeholder">
            <span class="album-art-hint">专辑封面</span>
          </div>

          <div class="popup-metadata">
            <p v-for="(line, i) in metadataLines" :key="i" class="metadata-line">
              {{ line }}
            </p>
          </div>
        </div>
      </div>
    </Teleport>
  </div>
</template>

<style scoped>
.each-song-row {
  display: grid;
  /* display: grid; 是 CSS 的网格布局（Grid Layout）属性，它把一个容器变成了“网格化”的二维布局系统——你可以像画表格一样，把子元素按行和列整齐排列 */
  grid-template-columns: 50px 1fr 150px 120px 80px 40px 40px;
  height: 70px;
  align-items: center;
  gap: 4px;
  padding: 0 5px;
  margin: 2px 5px;
  background-color: var(--colorCell);
  border-radius: var(--borderRadius);
  transition: background-color var(--easeTime) ease;
  user-select: none;
}

.each-song-row:hover {
  background-color: var(--colorHover);
}

/* ════════════════════════════════════════════════════════════════
   通用单元格
   ════════════════════════════════════════════════════════════════ */

.cell {
  display: flex;
  align-items: center;
  overflow: hidden;
}

/* ════════════════════════════════════════════════════════════════
   第 1 列：序号 / 播放状态
   ════════════════════════════════════════════════════════════════ */

.cell-ordinal {
  justify-content: center;
  flex-shrink: 0;
  cursor: pointer;
}

.ordinal-number {
  font-size: var(--midFont);
  color: var(--colorTextSecond);
}

.state-icon {
  width: 24px;
  height: 24px;
}

/* ════════════════════════════════════════════════════════════════
   第 2 列：歌名 + 艺术家堆叠
   min-width: 0 是让 ellipsis 生效的关键 ——
   没有它 flex/grid 子元素不会收缩到内容宽度以下
   ════════════════════════════════════════════════════════════════ */

.cell-name-artist {
  flex-direction: column;
  align-items: stretch;
  justify-content: center;
  gap: 2px;
  min-width: 0;
}

.name-row,
.artist-row {
  display: flex;
  align-items: center;
  gap: 6px;
  min-width: 0;
}

.song-name {
  font-size: var(--midFont);
  color: var(--colorTextMain);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  min-width: 0;
}

.artist-name {
  font-size: var(--littleFont);
  color: var(--colorTextSecond);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* ── 多流音频徽章：蓝底白字 ── */
.multi-stream-badge {
  flex-shrink: 0;
  background-color: var(--colorSuperStress);
  color: #ffffff;
  font-size: 14px;
  font-weight: bold;
  padding: 1px 6px;
  border-radius: 3px;
  white-space: nowrap;
  cursor: default;
}

/* ════════════════════════════════════════════════════════════════
   第 3-4 列：专辑、AI 分类 — 文本溢出省略号
   ════════════════════════════════════════════════════════════════ */

.ellipsis-text {
  display: block;
  width: 100%;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  font-size: var(--midFont);
  color: var(--colorTextSecond);
}

.cell-album,
.cell-genre {
  padding: 0 4px;
}

/* ════════════════════════════════════════════════════════════════
   第 5 列：播放次数
   ════════════════════════════════════════════════════════════════ */

.cell-play-count {
  justify-content: center;
}

.play-count-text {
  font-size: var(--littleFont);
  color: var(--colorTextSecond);
}

/* ════════════════════════════════════════════════════════════════
   第 6-7 列：喜欢按钮 / 更多信息按钮
   圆形点击区域，仿 C++ svgButton::paintButton 的悬停行为
   ════════════════════════════════════════════════════════════════ */

.cell-like,
.cell-more {
  justify-content: center;
  flex-shrink: 0;
  width: 30px;
  height: 30px;
  border-radius: 50%;
  cursor: pointer;
  transition: background-color var(--easeTime) ease;
}

.cell-like:hover,
.cell-more:hover {
  background-color: var(--colorHover);
}

.icon-btn {
  width: 20px;
  height: 20px;
  pointer-events: none;
}

/* ════════════════════════════════════════════════════════════════
   弹出窗 — Teleported to body
   仿 C++ popupWindow + PopupWindowButton 动画
   ════════════════════════════════════════════════════════════════ */

.popup-window {
  position: fixed;
  z-index: 10000;
  width: 400px;
  min-height: 350px;
  background-color: var(--colorMain);
  border-radius: 7px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.25);

  /*
    变换原点通过 JS 动态设置（按钮中心在 viewport 中的坐标），
    模拟 C++ AffineTransform::scale(progress, progress, cx, cy)
  */
  transform-origin: var(--popup-origin-x, center) var(--popup-origin-y, center);

  /* v-if 挂载时自动触发打开动画 */
  animation: popup-open 200ms ease-out forwards;
}

.popup-window.popup-closing {
  animation: popup-close 200ms ease-in forwards;
}

@keyframes popup-open {
  from {
    transform: scale(0);
    opacity: 0;
  }
  to {
    transform: scale(1);
    opacity: 1;
  }
}

@keyframes popup-close {
  from {
    transform: scale(1);
    opacity: 1;
  }
  to {
    transform: scale(0);
    opacity: 0;
  }
}

/* ── 标题栏 ── */

.popup-titlebar {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 32px;
  padding: 0 8px;
  position: relative;
  cursor: move;
  user-select: none;
}

.popup-title {
  font-size: var(--midFont);
  font-weight: bold;
  color: var(--colorTextMain);
  max-width: calc(100% - 32px);
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
}

.popup-close-btn {
  position: absolute;
  right: 4px;
  top: 50%;
  transform: translateY(-50%);
  width: 24px;
  height: 24px;
  border: none;
  background: transparent;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: var(--borderRadius);
  padding: 2px;
  transition: background-color var(--easeTime) ease;
}

.popup-close-btn:hover {
  background-color: var(--colorHover);
}

.popup-close-btn img {
  width: 100%;
  height: 100%;
}

/* ── 内容区域 ── */

.popup-content {
  padding: 10px;
}

/* 专辑封面占位（后续通过 imageHash 桥接获取） */
.popup-album-art-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 200px;
  height: 200px;
  margin: 0 auto 10px;
  border-radius: var(--borderRadius);
  background-color: var(--colorHover);
}

.album-art-hint {
  font-size: var(--littleFont);
  color: var(--colorTextSecond);
}

/* 元数据文本 */
.popup-metadata {
  font-size: var(--littleFont);
  color: var(--colorTextMain);
  line-height: 1.7;
  max-height: 280px;
  overflow-y: auto;
}

.metadata-line {
  margin: 2px 0;
  word-break: break-all;
}
</style>
