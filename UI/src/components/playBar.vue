<script setup lang="ts">
import { usePlayBackStore } from '@/store/playBackStore';
import { useSongStore } from '@/store/songStore';
import { IconHeart, IconHeartFilled, IconMessageCircleQuestion } from '@tabler/icons-vue';
import { getBackendResourceAddress } from 'juce-framework-frontend-mirror';
import PopupWindow from './other/popupWindow.vue';
import SongDetailInfo from './cell/songDetailInfo.vue';
import { computed } from 'vue';

const playBackStore = usePlayBackStore();
const songStore = useSongStore();
const info = computed(() => {
  const id = playBackStore.currentSongId;
  return songStore.songs.find((s) => s.songId === id);
});
</script>

<template>
  <div class="playbar-layout">
    <div class="left-area">
      <img :src="getBackendResourceAddress(`songId/${playBackStore.currentSongId}/image/50x50`)" />
      <div class="song-name-artist">
        <span
          :title="info?.title"
          style="color: var(--color-text-main); font-size: var(--mid-font)"
          >{{ info?.title }}</span
        >
        <span style="color: var(--color-text-second); font-size: var(--little-font)">{{
          info?.artist ?? '未知'
        }}</span>
      </div>

      <div class="cell-like" @click.stop="songStore.toggleMyLike(playBackStore.currentSongId!)">
        <IconHeartFilled v-if="info?.isMyLike" color="red" class="svg-icon" />
        <IconHeart v-else class="svg-icon" />
      </div>

      <PopupWindow title="歌曲详情">
        <template #trigger-button>
          <div class="cell-more">
            <IconMessageCircleQuestion class="svg-icon" />
          </div>
        </template>

        <template #popup-window-component>
          <SongDetailInfo v-if="info" :song="info"></SongDetailInfo>
        </template>
      </PopupWindow>
    </div>
    <div class="mid-area"></div>
    <div class="right-area"></div>
  </div>
</template>

<style lang="css" scoped>
.playbar-layout {
  position: fixed;
  left: 0;
  right: 0;
  bottom: 0;
  height: 90px;
  z-index: 1000;
  background: var(--color-hover);
  padding: 10px 30px;
  display: grid;
  grid-template-columns: 500px 1fr 330px 1fr 330px;
}

.left-area {
  grid-column: 1/2;
  padding: 0 10px;
  gap: 10px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
}

.song-name-artist {
  display: flex;
  flex-direction: column;
  /* 将主轴方向从水平（默认）改为垂直。 */
  align-items: flex-start;
  /* align开头的是交叉轴，justify开头的是主轴 */
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

.svg-icon {
  width: 32px;
  height: 32px;
  color: var(--color-text-main);
}

.mid-area {
  padding: 0 10px;
  gap: 10px;
  display: flex;
}

.right-area {
  padding: 0 10px;
  gap: 10px;
  display: flex;
  justify-content: flex-end;
  align-items: center;
}
</style>
