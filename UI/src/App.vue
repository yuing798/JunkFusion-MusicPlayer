<script setup lang="ts">
import { ref, watch, onMounted, type Component } from 'vue';
import LeftColumn from './components/LeftColumn.vue';
import AllMusic from './components/pages/AllMusic.vue';

//应用初始化的时候执行一次,不需要放到scripts的末尾
onMounted(() => {
  resolvedComponent(currentPageId.value);
});

const currentTheme = ref(localStorage.getItem('theme') || 'theme-light'); //默认亮色背景

// 监听主题变化，持久化到 localStorage
watch(
  currentTheme,
  (newTheme) => {
    localStorage.setItem('theme', newTheme);
    document.documentElement.className = newTheme;
  },
  { immediate: true },
);
//document.documentElement.className = newTheme的实际执行逻辑
// 假设当前：<html class="theme-light lang-zh">
// 执行 document.documentElement.className = 'theme-dark'
// 结果：<html class="theme-dark">

// ── 页面切换逻辑 ──
//   0 → AllMusic    1 → MyLike       2 → RecentPlay
//   3 → Artist      4 → Album       5 → Playlist
//   6 → Genre       7 → AIAssistant  8 → Effects
//   9 → Equalizer   10 → SpeakerArray 11 → Settings
const pageIdToComponent: Record<number, Component | null> = {
  0: AllMusic, // 所有音乐
  // 其他页面尚未开发，留 null
};

const currentPageId = ref<number | null>(0); //默认在所有歌曲这一页

/** 监听左侧导航栏的选中事件，切换主区域显示的页面 */
function handlePageChange(id: number): void {
  currentPageId.value = id;
}

/** 根据选中的 id 解析要渲染的组件，未开发的页面返回 null */
function resolvedComponent(id: number | null): Component | null {
  if (id === null) return null;
  return pageIdToComponent[id] ?? null;
}
// ?? 是 JavaScript/TypeScript 的“空值合并运算符（Nullish Coalescing Operator）”，
// 它的作用是：如果左边的值是 null 或 undefined，就取右边的值；否则就取左边的值。
</script>

<template>
  <div class="app-layout">
    <!-- 左侧导航栏：对应 C++ 中的 LeftColumn 组件
         @selection-changed 接收子组件传上来的页面 id -->
    <LeftColumn @selection-changed="handlePageChange" />

    <main class="main-content">
      <component v-if="resolvedComponent(currentPageId)" :is="resolvedComponent(currentPageId)" />
      <div v-else class="placeholder">
        <p>主内容区域</p>
        <p class="hint">（选择左侧导航以查看页面）</p>
      </div>
    </main>
  </div>
</template>

<style>
/* ================================================================
   全局重置 & 基础样式
   ================================================================ */

* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

html,
body {
  height: 100%;
  overflow: hidden;
}

#app {
  height: 100vh;
}

/* ================================================================
   应用布局：左侧导航栏 + 右侧主内容区
   对应 C++ 的整体布局结构
   ================================================================ */

.app-layout {
  display: flex;
  height: 100%;
}

/* ================================================================
   主内容区域（占位用）
   ================================================================ */

.main-content {
  flex: 1;
  /* 自动计算剩余的空间并全部分配给该区域 */
  display: flex;
  /* 水平和垂直居中 */
  background-color: var(--color-main);
  padding: 15px 20px;
}

/* 默认占位内容 */
.placeholder {
  text-align: center;
  color: var(--color-text-second);
  font-size: var(--big-font);
  user-select: none;
}

.placeholder .hint {
  margin-top: 8px;
  font-size: var(--little-font);
  color: var(--color-edge);
}
</style>
