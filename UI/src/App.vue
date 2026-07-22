<script setup lang="ts">
import { ref, watch, onMounted, type Component } from 'vue';
import LeftColumn from './components/LeftColumn.vue';
import AllMusic from './components/pages/AllMusic.vue';
import { useSongStore } from './store/songStore.ts';
import InfoWindow from './components/other/infoWindow.vue';
import { usePlayBackStore } from './store/playBackStore.ts';

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

const songStore = useSongStore();
const playBackStore = usePlayBackStore();

onMounted(() => {
  songStore.getAllSongs();
});
</script>

<template>
  <!-- 左侧导航栏：对应 C++ 中的 LeftColumn 组件
         @selection-changed 接收子组件传上来的页面 id -->
  <LeftColumn @selection-changed="handlePageChange" />

  <InfoWindow />
  <component v-if="resolvedComponent(currentPageId)" :is="resolvedComponent(currentPageId)" />
  <template v-else>
    <p>主内容区域</p>
    <p class="hint">（选择左侧导航以查看页面）</p>
  </template>
  <Transition name="slide-up">
    <!-- 自动监听v-if改变的瞬间，并施加动画效果 -->
    <playBar v-if="playBackStore.currentSongId !== null"></playBar>
    <!-- v-if放在外层而非内层的原因是如果放在内层的话即使if为false,script中也会执行，造成不必要的开销 -->
  </Transition>
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

/* 1. 动画进行时的状态：告诉浏览器哪些属性要过渡，时长和缓动函数 */
.slide-up-enter-active,
.slide-up-leave-active {
  transition:
    transform 0.5s cubic-bezier(0.25, 0.8, 0.25, 1),
    opacity 0.4s ease;

  /* 参数拆解：cubic-bezier(P1x, P1y, P2x, P2y)
  你给的 (0.25, 0.8, 0.25, 1) 对应两个控制点：
  点 1（P1）：(0.25, 0.8) —— 控制开头的速度（X=0.25 时，Y 已经达到了 0.8，说明起步极快）。
  点 2（P2）：(0.25, 1) —— 控制结尾的速度（X=0.25 时，Y 才刚到 1，说明最后的 75% 时间都在缓慢微调）。 */
  /* -active 是 Vue <Transition> 组件在动画执行期间（持续阶段） 添加的类名。它的核心职责只有一个：
    定义动画持续多久、延迟多久、以及用什么缓动函数（即上面说的 cubic-bezier） */
}

/* 2. 进入前（隐藏状态）& 离开后（隐藏状态） */
.slide-up-enter-from,
.slide-up-leave-to {
  transform: translateY(100%); /* 整个播放栏向下移动自身高度（即完全移出屏幕底部） */
  opacity: 0;
}

/* 3. 进入后（显示状态）& 离开前（显示状态） */
.slide-up-enter-to,
.slide-up-leave-from {
  transform: translateY(0); /* 恢复到正常位置（紧贴底部） */
  opacity: 1;
}
</style>
