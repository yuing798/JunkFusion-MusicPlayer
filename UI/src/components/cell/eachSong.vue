<script setup lang="ts">
import { computed } from 'vue';
import PopupWindow from '@/components/other/popupWindow.vue';
import { PlaybackState } from '@/macro/playState';

// ── Tabler 图标 ──
import {
  IconPlayerPlayFilled,
  IconPlayerPause,
  IconHeart,
  IconHeartFilled,
  IconMessageCircleQuestion,
} from '@tabler/icons-vue';
import { songStore, type SongInfo } from '@/store/songStore';
import { getBackendResourceAddress } from 'juce-framework-frontend-mirror';

const props = defineProps<{
  /** 歌曲数据库主键，用于从 store 查找完整 SongInfo 和计算序号 */
  songId: number;
  /** 当前播放状态 */
  playbackState: PlaybackState;
}>();

// ════════════════════════════════════════════════════════════════
// Store — 通过 songId 查找当前行的歌曲数据
// ════════════════════════════════════════════════════════════════

/** 从 store 中按 songId 查找 SongInfo，找不到返回 undefined */
const songInfo = computed(() => songStore.getSongById(props.songId));

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
         Stopped → 显示图片 ->@click.stop:进入playing状态
         Playing → play.svg ->@click.stop:进入paused状态
         Paused → pause.svg ->@click.stop:进入playing状态
         ═══════════════════════════════════════════════════════════ -->
    <div class="playback-image-area" @click.stop="handleOrdinalClick">
      <div v-if="playbackState === PlaybackState.Stopped">
        <img
          :src="getBackendResourceAddress(`songId/${props.songId}/image/50x50`)"
          class="cover-img"
        />
        <IconPlayerPlayFilled class="hover-play"></IconPlayerPlayFilled>
      </div>

      <IconPlayerPlayFilled
        v-else-if="playbackState === PlaybackState.Playing"
        class="play-pause-icon"
      />
      <IconPlayerPause v-else class="play-pause-icon" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 2 列：歌名 + 艺术家 (flex: 1)
         垂直堆叠：上行 = 歌名 下行 = 艺术家名
         ═══════════════════════════════════════════════════════════ -->
    <div class="song-name-artist">
      <span class="song-artist-name">{{ songInfo?.title ?? '' }}</span>
      <!-- {{ }} 是 Vue 的插值语法，只能用在 HTML 模板（template） 中，作用是把数据渲染到页面上 -->
      <!-- ||：只要左边是假值（false、0、''、null、undefined），就用右边。
           ??：只有当左边是 null 或 undefined 时，才用右边（更精准）。 -->
      <span class="song-artist-name">{{ songInfo?.artist ?? '未知' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 3 列：专辑名称 (150px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-album">
      <span class="ellipsis-text">{{ songInfo?.album ?? '未知' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 4 列：AI 分类标签 (120px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-genre">
      <span class="ellipsis-text">{{ songInfo?.aiGenre ?? '' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 5 列：播放次数 (80px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-play-count">
      <span class="play-count-text">{{ songInfo?.hadPlayedNum }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 6 列：我喜欢按钮 (40px)
         isMyLike=true → IconHeartFilled（红心）
         isMyLike=false → IconHeart（空心）
         toggle 动作直接调用 store，不 emit
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-like" @click.stop="songStore.toggleMyLike(songId)">
      <IconHeartFilled v-if="songInfo?.isMyLike" :size="20" color="#dd6572" class="svg-icon" />
      <IconHeart v-else :size="20" class="svg-icon" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 7 列：更多信息按钮 (40px)
         点击触发 PopupWindow，仿 C++ PopupWindowButton 的缩放动画
         ═══════════════════════════════════════════════════════════ -->
    <PopupWindow :title="popupTitle">
      <template #trigger>
        <div class="cell-more">
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
  /* 分别为图片，歌名，专辑，AI标签，播放次数，我喜欢，更多 */
  height: 80px;
  align-items: center;
  gap: 4px;
  padding: 10px 5px;
  background-color: linear-gradient(to top, var(--color-edge), var(--color-main));
  transition: background-color var(--ease-time) ease;
  user-select: none;
  border-bottom: 3px solid var(--color-edge); /* 粗细 颜色 样式 */
}

.each-song-row:hover {
  background-color: linear-gradient(to top, var(--color-text-second), var(--color-main));
}

/* ════════════════════════════════════════════════════════════════
   第 1 列：图片 / 播放状态
   ════════════════════════════════════════════════════════════════ */

.playback-image-area {
  width: 50px;
  height: 50px;
  cursor: pointer;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

.cover-img {
  height: 36px;
  width: 36px;
  object-fit: cover;
  transition: filter var(--ease-time) ease;
}
.cover-img:hover {
  filter: brightness(0.3);
  /* 悬浮时图片颜色变浅 */
}

.hover-play {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  color: var(--color-text-main);
  opacity: 0;
  transition: opacity var(--ease-time) ease;
}
.hover-play:hover {
  opacity: 1;
  /* 只有当悬浮的时候才显示播放图标 */
}

.play-pause-icon {
  width: 100%;
  height: 100%;
  color: var(--color-text-main);
}

/* ════════════════════════════════════════════════════════════════
   第 2 列：歌名 + 艺术家堆叠
   min-width: 0 是让 ellipsis 生效的关键 ——
   没有它 flex/grid 子元素不会收缩到内容宽度以下
   ════════════════════════════════════════════════════════════════ */

.song-name-artist {
  flex-direction: column;
  /* 将主轴方向从水平（默认）改为垂直。 */
  align-items: flex-start;
  /* 靠左对齐 */
  justify-content: center;
  gap: 2px;
  min-width: 0;
  /* 强行覆盖为 0，允许此容器在空间不足时收缩，防止被内部长文本撑破，从而配合溢出省略号（ellipsis）生效。 */
}

/* .song-name-artist span的意思是让song-name-artist类里面的所有span组件都遵循这个效果 */
.song-name-artist span {
  max-width: 100%;
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
  display: block;
  /* 让该元素独占一行 */
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
  width: 36px;
  height: 36px;
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
