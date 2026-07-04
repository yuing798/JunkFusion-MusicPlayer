<script setup lang="ts">
import { ref } from 'vue'

// 使用 @ 别名引用 assets 中的 logo 图片
// 对应 C++ 中 BinaryData::junkfusion_png
import logoImage from '@/assets/image/junk-fusion.png'

// ── 侧边栏分组数据 ──
// 与 C++ 中 UIDesign/FontAbout/language.h 的定义一一对应
// 所有按钮按顺序编号 0-11，用于单选逻辑

interface NavButton {//interface相当于cpp的struct，NavButton就是结构体名称，是一个编译期结构体定义工具
  id: number
  text: string
}

interface NavSection {
  label: string
  buttons: NavButton[]
}

const sections: NavSection[] = [
  {
    // 对应 language.h: musicLibraryBrowseID = "曲库浏览"
    label: '曲库浏览',
    buttons: [
      { id: 0, text: '所有音乐' },   // allMusicID
      { id: 1, text: '我喜欢' },     // myLikeID
      { id: 2, text: '最近播放' },   // recentPlayID
    ],
  },
  {
    // 对应 language.h: categoryBrowseID = "分类浏览"
    label: '分类浏览',
    buttons: [
      { id: 3, text: '作者' },       // artistID
      { id: 4, text: '专辑' },       // albumID
      { id: 5, text: '歌单' },       // playlistID
      { id: 6, text: '风格' },       // genreID
    ],
  },
  {
    // 对应 language.h: featureSectionID = "功能板块"
    label: '功能板块',
    buttons: [
      { id: 7, text: 'AI助手' },     // aiAssistantID
      { id: 8, text: '效果器' },     // effectsID
      { id: 9, text: '均衡器' },     // equalizerID
      { id: 10, text: '音箱阵列' },  // speakerArrayID
      { id: 11, text: '设置' },      // settingsID
    ],
  },
]

// ── 选中状态 ──
// 默认选中第一个按钮（所有音乐），与 C++ 中 selectID 默认值 0 一致
const selectedId = ref(0)

/**
 * 收音机行为（Radio Button）：
 * - 点击已选中的按钮：不做任何事（不允许取消选中）
 * - 点击其他按钮：切换到该按钮
 * 与 C++ LeftSelectedComponent::buttonClicked 的逻辑完全一致
 */
function handleSelect(id: number): void {
  if (selectedId.value !== id) {
    selectedId.value = id
  }
}

/**
 * 判断某个按钮是否被选中
 */
function isSelected(id: number): boolean {
  return selectedId.value === id
}
</script>

<template>
  <!--
    LeftColumn — 音乐播放器左侧导航栏
    对应 C++ 中的 LeftColumn 类：
    - 外层是浅灰色背景的可滚动容器（Viewport）
    - 内部包含 Logo、分组标签、导航按钮
  -->
  <nav class="left-column">
    <div class="scroll-content">
      <!-- Logo 区域：对应 LeftSelectedComponent 中的 logo ImageComponent -->
      <div class="logo-area">
        <img :src="logoImage" alt="" class="logo-image"/>
        <!-- src前面的冒号：这是 Vue 的 v-bind 指令的简写。它的作用是：把引号里的内容当作 JavaScript 
        表达式来执行，而不是当作普通的字符串。
        加冒号（动态绑定）：:src="logoImage"
        Vue 会解析 logoImage 这个变量，获取它的值（比如 /assets/logo.png），然后传给 src 属性。
        结果：成功加载你导入的图片。
        alt代表图片加载失败时：比如网络断了、路径写错了，浏览器会显示这个 alt 里的文字，让用户知道这里本该有一张什么图。
         -->
      </div>

      <!--
        遍历三个分组，渲染标签 + 按钮
        对应 C++ 中 labelArray 和 buttons 的布局
      -->
      <div
        v-for="(section, sectionIndex) in sections"
        :key="sectionIndex"
        class="nav-section"
      >
        <!-- 因为这些按钮是硬编码的，不会增减，所以直接使用index就行了，不用分配id -->
        <!-- 根据 sections 数组，动态生成多个 <div>，每个 <div> 对应数组中的一个元素 -->
        <!-- 对于数组：v-for="(item, index) in items"
        // item = 当前元素（类型取决于数组元素）
        // index = 数字索引（0, 1, 2...）
        对于对象(遍历键值对)：v-for="(value, key, index) in object"
        // value = 属性值
        // key = 属性名（字符串）
        // index = 数字索引（0, 1, 2...） -->
        <!--
          分组标签：对应 YLabel
          上下各有一条 5px 的分隔线，对应 C++ 中 paths[] 绘制的线条
        -->
        <div class="section-label">{{ section.label }}</div>
        <!-- {{ }}（双花括号插值）：用于标签的内容区域（即开始标签和结束标签之间的文字部分）且只能动态渲染文本内容。
        :（冒号，即 v-bind）：用于标签的属性区域（即开始标签内部的 key="value" 部分）。 -->

        <!-- 分组内的按钮列表 -->
        <div class="button-list">
          <button
            v-for="button in section.buttons"
            :key="button.id"
            :class="['nav-button', { selected: isSelected(button.id) }]"
            @click="handleSelect(button.id)"
          >
            {{ button.text }}
          </button>
        </div>
      </div>
    </div>
  </nav>
</template>

<style scoped>

/* ================================================================
   LeftColumn — 对应 C++ LeftColumn 类
   作为 Viewport 的滚动容器，背景色 shallowGrey
   ================================================================ */

.left-column {
  width: 220px;
  height: 100vh;
  /* vh 是“视口高度（Viewport Height）”单位。
  1vh = 当前浏览器窗口可见高度 的 1%。 */
  background-color: var(--colorNav);
  overflow-y: auto;
  /* overflow控制溢出内容如何处理 */
  /* overflow的属性值：hidden溢出的内容被裁剪掉，看不见,scroll：强制显示滚动条 */
  /* auto智能显示：内容多了自动出滚动条，少了隐藏 */
  overflow-x: hidden;
  /* 隐藏滚动条但保持滚动功能（可选，保留默认滚动条也 OK） */
  scrollbar-width: thin;
  scrollbar-color: var(--colorStress) transparent;
  /* scrollbar-color 接受 两个颜色值，语法是 scrollbar-color: [滑块颜色] [轨道颜色]; */
}

.scroll-content {
  /* 底部留白，对应 C++ resized() 中 height 计算公式末尾的 +60 */
  padding-bottom: 60px;
}

.logo-area {
  height: 60px;
  padding: 5px;
  /* padding是内边距的意思 */
  display: flex;
  align-items: center;
  justify-content: center;
}

.logo-image {
  max-height: 100%;
  max-width: 100%;
  object-fit: contain;
  /* object的几个参数：fill拉伸图片强行填满容器，不管宽高比
  contain图片等比缩放，直到完整地、不加裁剪地放进容器里。容器可能会有空白边
  cover图片等比缩放，直到完全覆盖容器，不留空白。图片超出容器的部分会被裁剪掉。 */
}

/* ================================================================
   分组标签 — 对应 C++ YLabel
   上下分隔线对应 C++ paths[] 中绘制的线条
   ================================================================ */

.section-label {
  height: 50px;
  display: flex;
  align-items: center;
  justify-content: center;

  /* 上下分隔线：对应 C++ 中 paths[] 的 strokePath */
  border-top: var(--divider-width) solid var(--color-hover);
  border-bottom: var(--divider-width) solid var(--color-hover);

  font-size: 18px;
  color: var(--color-text);
  user-select: none;
}

/* ================================================================
   分组内的按钮间距区域
   ================================================================ */

.button-list {
  padding: 10px 0;
}

/* ================================================================
   导航按钮 — 对应 C++ selectedButton
   宽度: 按钮区域宽 - 3px*2 的 reduced
   ================================================================ */

.nav-button {
  display: block;
  width: calc(100% - 6px);
  height: 40px;
  margin: 0 3px;

  border: none;
  border-radius: var(--button-radius);
  background-color: transparent;

  font-size: 18px;
  color: var(--color-text);
  text-align: center;
  line-height: 40px;

  cursor: pointer;
  user-select: none;

  /*
   过渡动画：让悬停和选中的颜色切换更平滑
   C++ 中没有这个过渡，但前端这样做体验更好
  */
  transition: background-color 0.15s ease;
}

/* 悬停态 — 对应 C++ selectedButton::paintButton 中的 shouldDrawButtonAsHighlighted */
.nav-button:hover:not(.selected) {
  background-color: var(--color-hover);
}

/* 选中态 — 对应 C++ selectedButton::paintButton 中 getToggleState() == true */
.nav-button.selected {
  background-color: var(--color-selected);
  /* 选中时文字仍然保持黑色，与 C++ 行为一致 */
}
</style>
