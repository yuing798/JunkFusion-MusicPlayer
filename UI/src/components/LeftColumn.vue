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
      v-for="section in sections"
      :key="section.label"
      class="navSection"
      >
        <div class="groupLabel">{{ section.label }}</div>

        <!-- 内层循环：遍历当下分组里面的按钮 -->
        <button 
        v-for="button in section.buttons"
        :key="button.id"
        class="navButton"
        :class="{active:isSelected(button.id)}"
        @click="handleSelect(button.id)"
        >{{ button.text }}</button>
      <!-- { active: ... }：这是一个 JavaScript 对象字面量。键（active）是你要添加的类名，
      值（selectedId === button.id）是一个布尔表达式。
      当 selectedId === button.id 为 true 时，这个按钮的 class 属性里会多出一个 active；
      为 false 时，没有 active。 

      @click="selectButton(button.id)" —— 事件绑定
      含义：当用户点击这个按钮时，执行 selectButton 方法，并把当前按钮的 id 作为参数传进去。

      :class 是“根据状态改变外观”的渲染逻辑（相当于 paint 中的 if 分支）。

      @click 是“改变状态的输入事件”（相当于 mouseDown 回调）。
      -->
      </div>
    </div>
  </nav>
</template>

<style scoped>
/* 当 <style> 标签带有 scoped attribute 的时候，它的 CSS 只会影响当前组件的元素 */

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

/* 标签区域 */
.groupLabel {
  height: 50px;
  display: flex;
  align-items: center;
  justify-content: center;

  /* 上下分隔线：对应 C++ 中 paths[] 的 strokePath */
  border-top: 5px solid var(--colorEdge);
  border-bottom: 5px solid var(--colorEdge);
  /* border-top: var(--divider-width) solid var(--color-hover); 
  是 border-width（粗细）、border-style（样式）、border-color（颜色） */

  font-size: var(--bigFont);
  font-weight: bold;
  color: var(--colorTextMain);
  user-select: none;
  /* user-select: none; 的意思是：禁止用户用鼠标选中该元素上的文本。
  在你正在开发的左侧导航栏（LeftColumn）中，如果用户在按钮文字上快速双击或拖拽鼠标，
  浏览器默认会高亮选中的文字（背景变蓝）。 */
}

/* ================================================================
   导航按钮 — 对应 C++ selectedButton
   宽度: 按钮区域宽 - 3px*2 的 reduced
   ================================================================ */

.navButton {
  display: block;
  /* display 决定了这个元素在页面布局中以什么身份参与“流式布局”。

  display: block;：让元素独占一整行，宽度默认填满父容器。

  display: inline;：元素不换行，宽度由内容撑开（类似 <span>）。

  display: inline-block;：结合两者，不换行但可以设置宽高。 */
  width: calc(100% - 6px);
  height: 40px;
  margin: 4px 4px;
  /* 问题三：为什么 margin 有两个参数？
  这是 CSS 的简写语法（Shorthand）。

  1 个参数：margin: 10px; → 上下左右都是 10px。

  2 个参数：margin: 0 3px; → 第一个是上下（0），第二个是左右（3px）。

  4 个参数：margin: 0 3px 5px 10px; → 上、右、下、左（顺时针）。 */

  border: none;
  border-radius: var(--borderRadius);
  /* border: none;：去掉边框线（视觉上没有任何描边）。

  border-radius: var(--button-radius);：把按钮的四个角切成圆角。 */
  background-color: transparent;
  /* background-color：元素的背景色（填充色），相当于 JUCE 里的 g.fillAll() 填充的颜色。

  color：元素内部文字的颜色，相当于 JUCE 里 g.setColour() 后 g.drawText() 用的颜色。 */

  font-size: var(--midFont);
  color: var(--colorTextMain);
  text-align: center;
  line-height: 40px;
  /* 一行的高度 */

  cursor: pointer;
  /* cursor 控制鼠标指针悬停在这个元素上时显示的形状。

  cursor: pointer; → 手形（👆），通常用于按钮和链接。

  cursor: default; → 默认箭头（➡️）。

  cursor: text; → 文本光标（I 形）。 */
  user-select: none;

  /* user-select: none; 就是禁止用户通过鼠标拖拽或双击选中该元素内的文字。

  没有这行：在按钮上双击，文字会变成蓝底白字（高亮）。

  有这行：双击时没有任何反应，就像在桌面原生应用里点按钮一样。 */

  /*
   过渡动画：让悬停和选中的颜色切换更平滑
   C++ 中没有这个过渡，但前端这样做体验更好
  */
  transition: background-color var(--easeTime) ease, transform var(--easeTime) ease, box-shadow var(--easeTime) ease;

  /* transition: background-color 0.15s ease; 拆解为：

  background-color：监听背景色的变化。

  0.15s：变化过程持续 150 毫秒。

  ease：缓动函数，先快后慢，让动画更自然 */
}

/* 悬停态 — 对应 C++ selectedButton::paintButton 中的 shouldDrawButtonAsHighlighted */
.navButton:hover:not(.active) {
  background-color: var(--colorHover);
  transform: translateY(1px);
  box-shadow: var(--shadow);
  /* box-shadow: [水平偏移] [垂直偏移] [模糊半径] [扩散半径] [颜色] */
}

/* 选中态 — 对应 C++ selectedButton::paintButton 中 getToggleState() == true */
.navButton.active {
  background-color: var(--colorStress);
  /* 选中时文字仍然保持黑色，与 C++ 行为一致 */
}
</style>
