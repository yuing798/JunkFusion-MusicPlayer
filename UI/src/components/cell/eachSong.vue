<script setup lang="ts">
import { computed } from 'vue';
import PopupWindow from '@/components/other/popupWindow.vue';
import { PlaybackState } from '@/macro/playState';
import { songStore } from '@/store/songStore';

// ── Tabler 图标 ──
import {
  IconPlayerPlayFilled,
  IconPlayerPause,
  IconHeart,
  IconHeartFilled,
  IconMessageCircleQuestion,
} from '@tabler/icons-vue';
import type { SongInfo } from '@/store/SongInfo';

const props = defineProps<{
  /** 歌曲数据库主键，用于从 store 查找完整 SongInfo 和计算序号 */
  songId: number;
  /** 当前播放状态 */
  playbackState: PlaybackState;
}>();

// ════════════════════════════════════════════════════════════════
// Store — 通过 songId 查找当前行的歌曲数据
// ════════════════════════════════════════════════════════════════

const mySongStore = songStore();

/** 从 store 中按 songId 查找 SongInfo，找不到返回 undefined */
const songInfo = computed(() => mySongStore.getSongById(props.songId));

/** 多流音频悬停提示文本 */
const MULTI_STREAM_TOOLTIP_TEXT =
  '该文件包含多路音频流（如多语言、多声道）。\
  当前播放器将自动为您选择质量最佳的默认音轨。\
  如需切换其他音轨，请使用专业音频工具（如 MKVToolNix）自行调整文件封装顺序';

// ════════════════════════════════════════════════════════════════
// 歌曲元数据（弹出窗内容，与 C++ OtherSongInfoIntro 一致）
// ════════════════════════════════════════════════════════════════

function getSongMetadataLines(info: SongInfo): string[] {
  const lines: string[] = [];

  const addIf = (label: string, value: string | number | undefined): void => {
    if (value === undefined) return;
    const sv = String(value);
    if (sv !== '' && sv !== '0' && sv !== '-1') {
      lines.push(`${label}: ${sv}`);
    }
  };

  addIf('BPM', info.bpm);
  addIf('调性', info.key);
  if (info.sampleRate > 0) addIf('采样率', `${info.sampleRate} Hz`);
  if (info.bitRate > 0) addIf('比特率', `${info.bitRate} kbps`);
  addIf('通道数', info.numChannels);
  addIf('位深', info.bitDepth);
  addIf('解码器名称', info.codecName);
  lines.push(`是否已经进行过AI分析: ${info.aiProcessed ? '是' : '否'}`);
  addIf('AI分析体裁', info.aiGenre);
  lines.push(`是否为音乐资源: ${info.isMusic ? '是' : '否'}`);
  addIf('专辑艺术家', info.albumArtist);
  addIf('体裁', info.genre);
  if (info.trackNumber !== undefined && info.trackNumber >= 0) addIf('轨道号', info.trackNumber);
  if (info.discNumber !== undefined && info.discNumber > 0) addIf('碟片号', info.discNumber);
  addIf('发行年份', info.year);
  addIf('作曲者', info.composer);
  addIf('文件路径', info.filePath);
  addIf('文件大小', info.fileSize);
  addIf('最后修改时间', info.lastModifiedTime);

  return lines;
}

const metadataLines = computed(() => {
  if (!songInfo.value) return [];
  return getSongMetadataLines(songInfo.value);
});

/** 弹出窗标题（songInfo 不存在时兜底） */
const popupTitle = computed(() => songInfo.value?.title || '未知');

/**
 * 点击序号/播放状态列的切换逻辑：
 *   Stopped → 请求进入 Playing
 *   Playing → 请求进入 Paused
 *   Paused  → 请求进入 Playing
 */
function handleOrdinalClick(): void {
  let nextState: PlaybackState;
  switch (props.playbackState) {
    case PlaybackState.Stopped:
      nextState = PlaybackState.Playing;
      break;
    case PlaybackState.Playing:
      nextState = PlaybackState.Paused;
      break;
    case PlaybackState.Paused:
      nextState = PlaybackState.Playing;
      break;
  }
  // 只传 songId，不再传 SongInfo 对象
  // emit('request-playback-change', props.songId, nextState);//以后放到playbackStore中
}
</script>

<template>
  <!--
    EachSong — 单首歌曲行
    7 列 Grid 布局：图片资源/状态 | 歌名+艺术家+多流 | 专辑 | AI分类 | 播放次数 | 喜欢 | 更多
  -->
  <div class="each-song-row">
    <!-- ═══════════════════════════════════════════════════════════
         第 1 列：序号 / 播放状态 (50px)
         Stopped → 显示序号数字 ->@click.stop:进入playing状态
         Playing → play.svg ->@click.stop:进入paused状态
         Paused → pause.svg ->@click.stop:进入playing状态
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-ordinal" @click.stop="handleOrdinalClick">
      <span v-if="playbackState === PlaybackState.Stopped" class="song-image">
        <!-- {{ songIndex }} -->
      </span>
      <IconPlayerPlayFilled
        v-else-if="playbackState === PlaybackState.Playing"
        :size="24"
        class="svg-icon"
      />
      <IconPlayerPause v-else :size="24" class="svg-icon" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 2 列：歌名 + 艺术家 + 多流徽章 (flex: 1)
         垂直堆叠：上行 = 歌名 + 多流徽章，下行 = 艺术家名
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-name-artist">
      <div class="name-row">
        <Tooltip :text="songInfo?.title ?? ''">
          <span class="song-name">{{ songInfo?.title ?? '' }}</span>
        </Tooltip>
        <Tooltip :text="MULTI_STREAM_TOOLTIP_TEXT">
          <span v-if="songInfo?.isMultiStreamFile" class="multi-stream-badge"> 多流音频 </span>
        </Tooltip>
      </div>
      <div class="artist-row">
        <Tooltip :text="songInfo?.artist ?? '未知'">
          <span class="artist-name">{{ songInfo?.artist ?? '未知' }}</span>
        </Tooltip>
      </div>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 3 列：专辑名称 (150px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-album">
      <span class="ellipsis-text">{{ songInfo?.album ?? '未知' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 4 列：AI 分类标签 (120px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-genre">
      <span class="ellipsis-text">{{ songInfo?.aiGenre ?? '' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 5 列：播放次数 (80px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-play-count">
      <span class="play-count-text">{{ songInfo?.hadPlayedNum ?? 0 }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 6 列：我喜欢按钮 (40px)
         isMyLike=true → IconHeartFilled（红心）
         isMyLike=false → IconHeart（空心）
         toggle 动作直接调用 store，不 emit
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell cell-like" @click.stop="mySongStore.toggleMyLike(songId)">
      <IconHeartFilled v-if="songInfo?.isMyLike" :size="20" color="#dd6572" class="svg-icon" />
      <IconHeart v-else :size="20" class="svg-icon" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 7 列：更多信息按钮 (40px)
         点击触发 PopupWindow，仿 C++ PopupWindowButton 的缩放动画
         ═══════════════════════════════════════════════════════════ -->
    <PopupWindow :title="popupTitle">
      <template #trigger>
        <div class="cell cell-more">
          <IconMessageCircleQuestion :size="20" class="svg-icon" />
        </div>
      </template>

      <template #default>
        <div class="song-detail">
          <!-- 专辑封面：后续通过 imageHash 桥接获取 -->
          <div class="song-detail-album-art">
            <span class="song-detail-album-art-hint">专辑封面</span>
          </div>

          <div class="song-detail-metadata">
            <p v-for="(line, i) in metadataLines" :key="i" class="song-detail-metadata-line">
              {{ line }}
            </p>
          </div>
        </div>
      </template>
    </PopupWindow>
  </div>
</template>

<style scoped>
.each-song-row {
  display: grid;
  /* display: grid; 是 CSS 的网格布局（Grid Layout）属性，它把一个容器变成了"网格化"的二维布局系统——你可以像画表格一样，把子元素按行和列整齐排列 */
  grid-template-columns: 50px 1fr 150px 120px 80px 40px 40px;
  height: 70px;
  align-items: center;
  gap: 4px;
  padding: 10px 5px;
  background-color: var(--color-cell);
  transition: background-color var(--ease-time) ease;
  user-select: none;
}

.each-song-row:hover {
  background-color: var(--color-hover);
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
   SVG 图标：不拦截鼠标事件（由父按钮处理）
   ════════════════════════════════════════════════════════════════ */

.svg-icon {
  pointer-events: none;
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
  font-size: var(--mid-font);
  color: var(--color-text-second);
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
  font-size: var(--mid-font);
  color: var(--color-text-main);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  min-width: 0;
}

.artist-name {
  font-size: var(--little-font);
  color: var(--color-text-second);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

/* ── 多流音频徽章：蓝底白字 ── */
.multi-stream-badge {
  flex-shrink: 0;
  background-color: var(--color-super-stress);
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
  font-size: var(--mid-font);
  color: var(--color-text-second);
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
  font-size: var(--little-font);
  color: var(--color-text-second);
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
  transition: background-color var(--ease-time) ease;
}

.cell-like:hover,
.cell-more:hover {
  background-color: var(--color-hover);
}

/* ════════════════════════════════════════════════════════════════
   弹出窗内容区域 — 歌曲详情
   由 PopupWindow 的 #default 插槽投射
   ════════════════════════════════════════════════════════════════ */

/* 专辑封面占位（后续通过 imageHash 桥接获取） */
.song-detail-album-art {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 200px;
  height: 200px;
  margin: 0 auto 10px;
  border-radius: var(--border-radius);
  background-color: var(--color-hover);
}

.song-detail-album-art-hint {
  font-size: var(--little-font);
  color: var(--color-text-second);
}

/* 元数据文本 */
.song-detail-metadata {
  font-size: var(--little-font);
  color: var(--color-text-main);
  line-height: 1.7;
  max-height: 280px;
  overflow-y: auto;
}

.song-detail-metadata-line {
  margin: 2px 0;
  word-break: break-all;
}
</style>
