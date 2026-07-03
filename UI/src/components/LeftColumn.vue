<script setup lang="ts">
import { ref } from 'vue'

// 使用 @ 别名引用 assets 中的 logo 图片
// 对应 C++ 中 BinaryData::junkfusion_png
import logoImage from '@/assets/junk-fusion.png'

// ── 侧边栏分组数据 ──
// 与 C++ 中 UIDesign/FontAbout/language.h 的定义一一对应
// 所有按钮按顺序编号 0-11，用于单选逻辑

interface NavButton {
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
        <img
          :src="logoImage"
          alt="Junk Fusion Logo"
          class="logo-image"
        />
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
        <!--
          分组标签：对应 YLabel
          上下各有一条 5px 的分隔线，对应 C++ 中 paths[] 绘制的线条
        -->
        <div class="section-label">{{ section.label }}</div>

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
   颜色变量 — 与 C++ YColor 结构体中的颜色值一一对应
   ================================================================ */

/* YColor::shallowGrey — 侧边栏背景色 */
:root {
  --color-bg: #d8d6d6;
  /* YColor::midGrey — 悬停态背景、分隔线颜色 */
  --color-hover: #c5c3c3;
  /* YColor::greyBlue — 选中态背景 */
  --color-selected: #6e75dd;
  /* 文字颜色（YColor::black） */
  --color-text: #000000;
  /* 按钮圆角（C++ 中 fillRoundedRectangle 的 6.0f） */
  --button-radius: 6px;
  /* 分隔线粗细（C++ 中 PathStrokeType(5.0f)） */
  --divider-width: 5px;
}

/* ================================================================
   LeftColumn — 对应 C++ LeftColumn 类
   作为 Viewport 的滚动容器，背景色 shallowGrey
   ================================================================ */

.left-column {
  width: 220px;
  height: 100vh;
  background-color: var(--color-bg);
  overflow-y: auto;
  overflow-x: hidden;
  /* 隐藏滚动条但保持滚动功能（可选，保留默认滚动条也 OK） */
  scrollbar-width: thin;
  scrollbar-color: var(--color-hover) transparent;
}

.scroll-content {
  /* 底部留白，对应 C++ resized() 中 height 计算公式末尾的 +60 */
  padding-bottom: 60px;
}

/* ================================================================
   Logo 区域
   对应 C++:
     logo.setBounds(local.removeFromTop(60).reduced(5));
   ================================================================ */

.logo-area {
  height: 60px;
  padding: 5px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.logo-image {
  max-height: 100%;
  max-width: 100%;
  object-fit: contain;
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
