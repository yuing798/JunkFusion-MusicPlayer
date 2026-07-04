<script setup lang="ts">
/**
 * App.vue — Junk Fusion 音乐播放器根组件
 *
 * 当前阶段只展示左侧导航栏（LeftColumn），不涉及与 JUCE 的通信。
 * 主内容区域留空，后续逐步添加。
 */
import { ref, watch, onMounted } from 'vue'
import LeftColumn from './components/LeftColumn.vue'

const currentTheme = ref(localStorage.getItem('theme') || 'theme-light')

// 切换主题的函数
function toggleTheme() {
  currentTheme.value = currentTheme.value === 'theme-light' 
    ? 'theme-dark-gold' 
    : 'theme-light'
}
//两个等号==:宽松相等运算符,相当于 C++ 的 ==，但会偷偷做类型转换（比如 1 == '1' 结果为 true）。
//三个等号===:严格相等运算符,相当于 C++ 的 == + 编译期类型检查（禁止类型转换）
//判断相等优先级高于赋值

// 监听主题变化，持久化到 localStorage
watch(currentTheme, (newTheme) => {
  localStorage.setItem('theme', newTheme)
  document.documentElement.className = newTheme
},{immediate:true})
//监视 currentTheme 这个响应式变量。
// 一旦它变了，就自动执行后面的回调函数 (newTheme) => { ... }。
// 回调里把新值 newTheme 存进 localStorage。
//immediate: true表示函数在创建的时候立刻执行一次回调

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
        <p>主内容区域</p>
        <p class="hint">（后续逐步添加）</p>
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
/* 通配符选择器,匹配 页面上的每一个 HTML 标签（<div>、<p>、<button>、<ul>……所有） */
*::before,
*::after {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
  /* 写 width: 100px; 只是设置内容区的宽度。如果你加了 padding: 10px; 和 border: 2px;，这个盒子的实际总宽度会变成 100 + 10*2 + 2*2 = 124px
  border-box：你写 width: 100px; 就是设定最终总宽度。如果加了 padding 和 border，浏览器会自动向内压缩内容区，总宽度永远锁死在 100px。
  margin是外边距，padding是内边距 */
}

html, body {
  height: 100%;/* ① 让 html 和 body 撑满整个浏览器窗口的高度 */
  overflow: hidden; /* ② 禁止页面出现滚动条（裁剪溢出内容） */
}

#app {
  height: 100%;/* ③ 让 Vue 挂载点也撑满整个窗口高度 */
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
  align-items: center;
  justify-content: center;
  /* 水平和垂直居中 */
  background-color: var(--colorMain); 
}
</style>
