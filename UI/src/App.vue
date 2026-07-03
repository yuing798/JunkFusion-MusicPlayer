<script setup lang="ts">
/**
 * App.vue — Junk Fusion 音乐播放器根组件
 *
 * 当前阶段只展示左侧导航栏（LeftColumn），不涉及与 JUCE 的通信。
 * 主内容区域留空，后续逐步添加。
 */
import LeftColumn from './components/LeftColumn.vue'
</script>

<template>
  <div class="app-layout">
    <!-- 左侧导航栏：对应 C++ 中的 LeftColumn 组件 -->
     <nav>
        <LeftColumn />
     </nav>
    

    <!--
      主内容区域（占位）
      后续会添加播放器主体、右侧面板等内容
    -->
    <main class="main-content">
      <div class="placeholder">
        <p>主内容区域</p>
        <p class="hint">（后续逐步添加）</p>
      </div>
    </main>
    <footer>
        <!--这里放置歌曲切换与选择部分-->
    </footer>
  </div>
</template>

<style>
/* ================================================================
   全局重置 & 基础样式
   ================================================================ */

*,
*::before,
*::after {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

html, body {
  height: 100%;
  overflow: hidden;
}

#app {
  height: 100%;
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
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: #f9f9f9;  /* YColor::white */
}

.placeholder {
  text-align: center;
  color: #ababab;  /* YColor::darkGrey */
  font-size: 18px;
  user-select: none;
}

.placeholder .hint {
  margin-top: 8px;
  font-size: 14px;
  color: #c5c3c3;  /* YColor::midGrey */
}
</style>
