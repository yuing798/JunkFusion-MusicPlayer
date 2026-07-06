<script setup lang="ts">
import { ref } from 'vue'

// 使用 @ 别名引用 assets 中的 logo 图片
// 对应 C++ 中 BinaryData::junkfusion_png
import logoImage from '@/assets/image/junk-fusion.png'

// ── 向父组件发送事件 ──
// 对应 C++ 中 LeftSelectedComponent::onSelectionChanged 回调
const emit = defineEmits<{
  (e: 'selection-changed', id: number): void
}>()
//defineEmits	Vue 3 的编译器宏（Compiler Macro）。它不需要手动 import，
// 在 <script setup> 中直接可用。作用是注册当前组件允许向外触发的事件。

// ── 侧边栏分组数据 ──
// 与 C++ 中 UIDesign/FontAbout/language.h 的定义一一对应
// 所有按钮按顺序编号 0-11，用于单选逻辑

interface NavButton {
  //interface相当于cpp的struct，NavButton就是结构体名称，是一个编译期结构体定义工具
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
      { id: 0, text: '所有音乐' }, // allMusicID
      { id: 1, text: '我喜欢' }, // myLikeID
      { id: 2, text: '最近播放' }, // recentPlayID
    ],
  },
  {
    // 对应 language.h: categoryBrowseID = "分类浏览"
    label: '分类浏览',
    buttons: [
      { id: 3, text: '作者' }, // artistID
      { id: 4, text: '专辑' }, // albumID
      { id: 5, text: '歌单' }, // playlistID
      { id: 6, text: '风格' }, // genreID
    ],
  },
  {
    // 对应 language.h: featureSectionID = "功能板块"
    label: '功能板块',
    buttons: [
      { id: 7, text: 'AI助手' }, // aiAssistantID
      { id: 8, text: '效果器' }, // effectsID
      { id: 9, text: '均衡器' }, // equalizerID
      { id: 10, text: '音箱阵列' }, // speakerArrayID
      { id: 11, text: '设置' }, // settingsID
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
    // 通知父组件：选中页面已改变
    // 对应 C++: if (onSelectionChanged) onSelectionChanged(selectID);
    emit('selection-changed', id)
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
        <img :src="logoImage" alt="" class="logo-image" />
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
      <div v-for="section in sections" :key="section.label" class="navSection">
        <div class="groupLabel">{{ section.label }}</div>

        <!-- 内层循环：遍历当下分组里面的按钮 -->
        <button
          v-for="button in section.buttons"
          :key="button.id"
          class="navButton"
          :class="{ active: isSelected(button.id) }"
          @click="handleSelect(button.id)"
        >
          {{ button.text }}
        </button>
        <!-- { active: ... }：这是一个 JavaScript 对象字面量。键（active）是你要添加的类名，
      值（selectedId === button.id）是一个布尔表达式。
      当 selectedId === button.id 为 true 时，这个按钮的 class 属性里会多出一个 active；
      为 false 时，没有 active。 

      @click="selectButton(button.id)" —— 事件绑定
      含义：当用户点击这个按钮时，执行 selectButton 方法，并把当前按钮的 id 作为参数传进去。

      :class 是“根据状态改变外观”的渲染逻辑（相当于 paint 中的 if 分支）。
      :class 是 v-bind:class 的语法糖，意思是 “动态绑定 class”。
      等号右边不再是一个普通字符串，而是一个 JavaScript 表达式（可以是对象、数组或三目运算符）。
      Vue 会实时计算这个表达式的值，并把它“合并”到元素的 class 属性中。

      class="navButton active"：这是静态的。active 这个类永远存在，不管 isSelected 返回 true 还是 false，按钮永远高亮。
      :class="{ active: isSelected(button.id) }"：这是动态的。active 类跟随状态变化。
      当 isSelected 返回 true 时，按钮高亮；返回 false 时，高亮消失。

      :class="{isSelected(button.id) }"
      这是 Vue 模板中的语法错误！
      在 :class 的对象语法中，必须是 { 类名: 条件 } 的键值对结构。你漏写了键（active），直接放了一个表达式进去。

      @click 是“改变状态的输入事件”（相当于 mouseDown 回调）。-->
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
  /* 保持你原本的尺寸和基础定位 */
  width: calc(100% - 10px);
  height: 40px;
  margin: 4px 4px;
  position: relative; /* 必须加这句，用于伪元素绝对定位 */

  border: none;
  background-color: transparent;

  font-size: var(--midFont);
  color: var(--colorTextMain);
  text-align: center;
  line-height: 40px;
  /* 行距 */

  cursor: pointer;
  user-select: none;
  /* 用来控制用户能否用鼠标或手指选中页面上的文本 */

  /* 优化过渡属性，加入 clip-path 和 color 的平滑过渡 */
  transition:
    background-color var(--easeTime) ease,
    transform var(--easeTime) ease,
    clip-path var(--easeTime) ease,
    color var(--easeTime) ease,
    font-weight var(--easeTime) ease;
}

/* 悬停态 — 对应 C++ selectedButton::paintButton 中的 shouldDrawButtonAsHighlighted */
.navButton:hover:not(.active) {
  /* 保持你原本的悬浮位移 */
  transform: translateY(-3px);
  color: var(--colorTextMain); /* 悬浮时文字微亮 */

  /* 悬浮时赋予微弱的八角形轮廓和半透明背景 */
  background-color: rgba(255, 255, 255, 0.08);

  /* 八角矩形裁切 (四周切掉 8px 契合 40px 高度) */
  clip-path: polygon(
    8px 0,
    calc(100% - 8px) 0,
    100% 8px,
    100% calc(100% - 8px),
    calc(100% - 8px) 100%,
    8px 100%,
    0 calc(100% - 8px),
    0 8px
  );
}

.navButton.active {
  color: #ffffff;
  font-weight: 600;
  background-color: transparent; /* 把基底留给渐变 */

  /* 激活时的八角形裁切 */
  clip-path: polygon(
    8px 0,
    calc(100% - 8px) 0,
    100% 8px,
    100% calc(100% - 8px),
    calc(100% - 8px) 100%,
    8px 100%,
    0 calc(100% - 8px),
    0 8px
  );

  /* 多重渐变：135度高光掠影 + 底部逆向暗面 + 音乐软件常用的电音蓝紫主色调 */
  background-image:
    linear-gradient(135deg, rgba(255, 255, 255, 0.4) 0%, rgba(255, 255, 255, 0) 50%),
    linear-gradient(315deg, rgba(0, 0, 0, 0.3) 0%, rgba(0, 0, 0, 0) 40%),
    linear-gradient(180deg, #00f2fe 0%, #4facfe 100%);
}

/* 激活态下的内棱角光泽（使用 :before 伪元素） */
.navButton.active::before {
  content: '';
  position: absolute;
  top: 1px;
  left: 1px;
  right: 1px;
  bottom: 1px; /* 往内缩 1px 形成晶莹边缘 */
  z-index: -1;

  /* 内部缩进版的八角裁切 */
  clip-path: polygon(
    7px 0,
    calc(100% - 7px) 0,
    100% 7px,
    100% calc(100% - 7px),
    calc(100% - 7px) 100%,
    7px 100%,
    0 calc(100% - 7px),
    0 7px
  );

  /* 上亮下暗的微弱渐变，模拟宝石上边缘的锐利折射 */
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.5) 0%, rgba(255, 255, 255, 0) 60%);
  mix-blend-mode: overlay;
}

/* ==================== 3. 完美的八角外发光方案 ==================== */
/* 核心技巧：利用 :after 伪元素作为背景，超出 button 裁剪范围的部分通过 blur 变为发光 */
.navButton::after {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: -2; /* 放在最底层 */
  background: linear-gradient(180deg, #00f2fe, #4facfe);

  /* 同样进行八角裁切，确保发光形状也是八角宝石状 */
  clip-path: polygon(
    8px 0,
    calc(100% - 8px) 0,
    100% 8px,
    100% calc(100% - 8px),
    calc(100% - 8px) 100%,
    8px 100%,
    0 calc(100% - 8px),
    0 8px
  );

  /* 关键：使用高斯模糊打散成霓虹外发光 */
  filter: blur(8px);
  opacity: 0; /* 默认未激活、未悬浮时完全透明 */
  transition: opacity var(--easeTime) ease;
}

/* 激活状态下，让外发光显现 */
.navButton.active::after {
  opacity: 0.6; /* 强弱可以通过透明度控制 */
}

/* 激活状态下再悬浮，让发光额外变强一点 */
.navButton.active:hover::after {
  opacity: 0.8;
}
</style>
