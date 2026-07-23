<script setup lang="ts">
import { computed } from 'vue';

// ── Tabler 图标 ──
import {
  IconPlayerPlayFilled,
  IconPlayerPause,
  IconHeart,
  IconHeartFilled,
  IconMessageCircleQuestion,
} from '@tabler/icons-vue';
import { useSongStore, type SongInfo } from '@/store/songStore';
import { getBackendResourceAddress } from 'juce-framework-frontend-mirror';
import { usePlayBackStore } from '@/store/playBackStore';
import SongDetailInfo from './songDetailInfo.vue';
import PopupWindow from '@/components/other/popupWindow.vue';

const props = defineProps<{
  song: SongInfo;
}>();

const playBackStore = usePlayBackStore();
const songStore = useSongStore();

function changePlayBack() {
  if (props.song.songId !== playBackStore.currentSongId) {
    playBackStore.isPlaying = true;
    playBackStore.setCurrentSongId(props.song.songId);
  } else {
    if (playBackStore.isPlaying === false) {
      playBackStore.isPlaying = true;
    } else {
      playBackStore.isPlaying = false;
    }
  }
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
    <div class="playback-image-area" @click.stop="changePlayBack()">
      <div v-if="props.song.songId !== playBackStore.currentSongId">
        <img
          :src="getBackendResourceAddress(`songId/${props.song.songId}/image/50x50`)"
          class="cover-img"
        />
        <IconPlayerPlayFilled class="hover-play"></IconPlayerPlayFilled>
      </div>

      <template v-else>
        <IconPlayerPause v-if="playBackStore.isPlaying === true" class="svg-icon"></IconPlayerPause>
        <IconPlayerPlayFilled v-else class="svg-icon"></IconPlayerPlayFilled>
      </template>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 2 列：歌名 + 艺术家 (flex: 1)
         垂直堆叠：上行 = 歌名 下行 = 艺术家名
         ═══════════════════════════════════════════════════════════ -->
    <div class="song-name-artist">
      <span
        style="color: var(--color-text-main); font-size: var(--mid-font)"
        :title="props.song.title"
        >{{ props.song.title }}</span
      >
      <span style="color: var(--color-text-second); font-size: var(--little-font)">{{
        props.song.artist ?? '未知'
      }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 3 列：专辑名称 (150px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-album">
      <span class="ellipsis-text">{{ props.song.album ?? '未知' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 4 列：AI 分类标签 (120px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-genre">
      <span class="ellipsis-text">{{ props.song.aiGenre ?? '' }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 5 列：播放次数 (80px)
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-play-count">
      <span class="play-count-text">{{ props.song.playNum }}</span>
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 6 列：我喜欢按钮 (40px)
         isMyLike=true → IconHeartFilled（红心）
         isMyLike=false → IconHeart（空心）
         ═══════════════════════════════════════════════════════════ -->
    <div class="cell-like" @click.stop="songStore.toggleMyLike(props.song.songId)">
      <IconHeartFilled v-if="props.song.isMyLike" color="red" class="svg-icon" />
      <IconHeart v-else class="svg-icon" />
    </div>

    <!-- ═══════════════════════════════════════════════════════════
         第 7 列：更多信息按钮 (40px)
         点击触发 PopupWindow，仿 C++ PopupWindowButton 的缩放动画
         ═══════════════════════════════════════════════════════════ -->
    <PopupWindow title="歌曲详情">
      <template #trigger-button>
        <div class="cell-more">
          <IconMessageCircleQuestion class="svg-icon" />
        </div>
      </template>

      <template #popup-window-component>
        <SongDetailInfo :song="props.song"></SongDetailInfo>
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
  height: 100%;
  width: 100%;
  align-items: center;
  gap: 4px;
  padding: 10px 5px;
  background: linear-gradient(to top, var(--color-edge), var(--color-main) 30%);
  transition: background-color var(--ease-time) ease;
  user-select: none;
  border-bottom: 3px solid var(--color-edge); /* 粗细 颜色 样式 */
}

.each-song-row:hover {
  background: linear-gradient(to top, var(--color-text-second), var(--color-main) 30%);
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
  height: 100%;
  width: 100%;
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

.svg-icon {
  width: 32px;
  height: 32px;
  color: var(--color-text-main);
}

/* ════════════════════════════════════════════════════════════════
   第 2 列：歌名 + 艺术家堆叠
   min-width: 0 是让 ellipsis 生效的关键 ——
   没有它 flex/grid 子元素不会收缩到内容宽度以下
   ════════════════════════════════════════════════════════════════ */

.song-name-artist {
  display: flex;
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
  display: flex;
  align-items: center;
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
</style>
