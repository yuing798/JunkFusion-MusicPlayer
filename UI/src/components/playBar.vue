<script setup lang="ts">
import { usePlayBackStore } from '@/store/playBackStore';
import { getBackendResourceAddress } from 'juce-framework-frontend-mirror';

const playBackStore = usePlayBackStore();
</script>

<template>
  <div class="playbar-layout">
    <div class="left-area">
      <img :src="getBackendResourceAddress(`songId/${playBackStore.currentSongId}/image/50x50`)" />
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
  grid-template-columns: 330px 1fr 330px 1fr 330px;
}

.left-area {
  grid-column: 1/2;
  padding: 0 10px;
  gap: 10px;
  display: flex;
}

.mid-area {
  grid-column: 3/4;
  padding: 0 10px;
  gap: 10px;
  display: flex;
}

.right-area {
  grid-column: 5/6;
  padding: 0 10px;
  gap: 10px;
  display: flex;
}
</style>
