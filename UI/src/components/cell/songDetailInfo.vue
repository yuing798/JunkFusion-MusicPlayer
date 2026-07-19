<script setup lang="ts">
import type { SongInfo } from '@/store/songStore';
import { getBackendResourceAddress } from 'juce-framework-frontend-mirror';
import { computed } from 'vue';

const props = defineProps<{
  song: SongInfo;
}>();

const s = props.song;
const techInfoArray = computed(() => [
  { label: '时长', value: s.duration },
  { label: '采样率', value: s.sampleRate },
  { label: '比特率', value: s.bitRate },
  { label: '通道数', value: s.numChannels },
  { label: '位深', value: s.bitDepth },
  { label: '解码器', value: s.codecName },
]);
const musicInfoArray = computed(() => [
  { label: '播放次数', value: s.playNum },
  { label: 'BPM', value: s.bpm },
  { label: '调性', value: s.key },
  { label: '体裁(原始)', value: s.genre },
  { label: '体裁(AI分析)', value: s.aiGenre },
  { label: '轨道号', value: s.trackNumber },
  { label: '碟片号', value: s.discNumber },
  { label: '发行年份', value: s.year },
  { label: '作曲家', value: s.composer },
  { label: '专辑艺术家', value: s.albumArtist },
]);
</script>

<template>
  <div class="area">
    <div class="left-column">
      <img :src="getBackendResourceAddress(`songId/${s.songId}/image/240x240`)" />
      <div style="display: flex; flex-direction: column">
        <button class="shiny-btn">编辑内容</button>
        <button class="shiny-btn">联网自动获取补全元数据</button>
      </div>
    </div>
    <div style="flex: 1; display: flex; flex-direction: column">
      <!-- 这里放置剩余的内容，可以视为right-column -->
      <div class="main-info">
        <p style="font-size: 21px">{{ s.title }}</p>
        <p>{{ s.artist }}</p>
        <p>{{ s.album }}</p>
      </div>
      <div class="meta-grid">
        <!-- 这里放置技术参数 -->
        <div v-for="item in techInfoArray" :key="item.label">
          <div v-if="item.value !== null">
            <span class="opt-label">{{ item.label }} </span>
            <span class="opt-value" :title="item.value">{{ item.value }}</span>
          </div>
        </div>
      </div>
      <div class="meta-grid">
        <!-- 这里放置音乐参数 -->
        <div v-for="item in musicInfoArray" :key="item.label">
          <div v-if="item.value !== null">
            <span class="opt-label">{{ item.label }} </span>
            <span class="opt-value">{{ item.value }}</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.area {
  /* 整篇弹窗的区域 */
  display: flex;
  justify-content: center;
  /* 水平居中 */

  width: 720px;
  padding: 10px;
  gap: 10px;
}

.left-colume {
  align-items: center;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.main-info {
  font-weight: bold;
  font-size: var(--mid-font);
}

.meta-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr); /* 3列等宽 */
  gap: 6px 32px; /* 行间距6px，列间距32px */
  padding: 8px 0;
  font-size: var(--mid-font);
  border-top: 1px solid var(--color-edge);
}

.opt-label {
  /* 可选字段的标签 */
  color: var(--color-text-second);
}
.opt-value {
  /* 可选字段的具体数值 */
  color: var(--color-text-main);
}
</style>
