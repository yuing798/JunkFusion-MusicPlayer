<script setup lang="ts">
import { ref } from 'vue'

// SVG 图标资源 — 对应 C++ BinaryData 中的 SVG 资源
import refreshSvg from '@/assets/image/refresh.svg'
import upSvg from '@/assets/image/up.svg'
import downSvg from '@/assets/image/down.svg'

// ── 标题标签 ──
// 对应 C++: BigLabel allMusicLabel{U("全部音乐")};
const pageTitle = '全部音乐'

// ── 歌曲数量标签 ──
// 对应 C++: littleLabel numSongsLabel{U("共 0 首")};
// 后续与 SongSelectViewport 对接时会被动态更新
const songCount = ref(0)

// ── 排序方式下拉框 ──
// 对应 C++: YComboBox seqWays
//   seqWays.addItem(U("歌曲名称排列"));
//   seqWays.addItem(U("添加时间排列"));
//   seqWays.addItem(U("播放次数排列"));
const sortOptions = [
     
  { value: 1, label: '添加时间排列' },  // SortMode::ByAddTime
  { value: 0, label: '歌曲名称排列' },   // SortMode::ByName
  { value: 2, label: '播放次数排列' },   // SortMode::ByPlayTimes
]
const selectedSort = ref(0)
// 映射到 SortMode 枚举：与 C++ indexToSortMode lambda 一致
function indexToSortMode(idx: number): string {
  switch (idx) {
    case 0:  return 'ByAddTime'
    case 1:  return 'ByName'
    case 2:  return 'ByPlayTimes'
    default: return 'ByAddTime'
  }
}

// ── 升降序切换按钮 ──
// 对应 C++: upDownButton mUpDownButton
//   bool upOrDown{false};  // false=升序, true=降序
//   mUpDownButton.setToggleState(upOrDown, ...);
const isAscending = ref(true)  // true = 升序，与 C++ ascendingWay 默认值一致

function toggleSortDirection(): void {
  isAscending.value = !isAscending.value
}

// ── 文件导入 ──
// 对应 C++: yTextButton selectFileButton{U("导入文件")};
// 在 web 环境中使用 <input type="file"> 替代 JUCE 的 FileChooser
// 注意：当前只做 UI 层，文件导入的后端逻辑后续对接

const isImporting = ref(false)
const fileInput = ref<HTMLInputElement | null>(null)
  //和cpp一样：<>代表模板，<HTMLInputElement | null> 告诉 TypeScript 编译器：
  // “这个响应式数据的 .value 类型，要么是 HTMLInputElement，要么是 null。”

function handleImportClick(): void {
  // 触发隐藏的 file input
  fileInput.value?.click()
}

async function handleFilesSelected(event: Event): Promise<void> {
  const input = event.target as HTMLInputElement
  if (!input.files || input.files.length === 0) return

  isImporting.value = true

  try {
    // 对应 C++:
    //   selectFileButton.setClickingTogglesState(false);
    //   getMultiMediaFileChoose(callback, this);
    // 实际文件导入逻辑留待后续与后端对接
    // await ......
    const files = Array.from(input.files)
    console.log(`[AllMusic] 选中 ${files.length} 个文件待导入:`, files.map(f => f.name))

    // 模拟导入完成 → 刷新
    // 后续接入 real API 后，这里替换为实际的元数据提取 + 数据库写入
  } finally {
    isImporting.value = false
    // 重置 input，以便再次选择相同文件时也能触发 change 事件
    if (input) input.value = ''
  }
}

// ── 刷新按钮 ──
// 对应 C++: svgButton refreshButton{U("刷新"), ...};
function handleRefresh(): void {
  // 对应 C++ 中刷新逻辑 — 后续与 SongSelectViewport 对接
  console.log('[AllMusic] refresh triggered')
}
</script>

<template>
  <!--
    AllMusicPage — "所有音乐" 页面
    对应 C++ 中的 AllMusicPage 类（仅重构该类，不包含 SongSelectViewport / PageChange）
  -->
  <div class="all-music-page">
    <!-- ================================================================
         第 1 行：标题行（对应 C++ resized() 中的 row1）
         allMusicLabel + numSongsLabel
         ================================================================ -->
    <div class="header-row">
      <!-- BigLabel: "全部音乐" — fontSize 30，与 C++ BigLabel 一致 -->
      <span class="page-title">{{ pageTitle }}</span>

      <!-- littleLabel: "共 0 首" — fontSize 15，与 C++ littleLabel 一致 -->
      <span class="song-count">共 {{ songCount }} 首</span>
    </div>

    <!-- ================================================================
         第 2 行：工具栏行（对应 C++ resized() 中的 row2）
         selectFileButton / refreshButton / seqWays combo / upDownButton
         ================================================================ -->
    <div class="toolbar-row">
      <!--
        "导入文件" 按钮 — 对应 C++ yTextButton selectFileButton
        宽度 80px，与 C++ row2.removeFromLeft(80) 一致
      -->
      <button
        class="btn-import"
        :disabled="isImporting"
        @click="handleImportClick"
      >
        {{ isImporting ? '导入中…' : '导入文件' }}
      </button>

      <!-- 隐藏的 file input，替代 JUCE FileChooser -->
      <input
        ref="fileInput"
        type="file"
        multiple
        accept="audio/*"
        style="display: none"
        @change="handleFilesSelected"
      />

      <!--
        刷新按钮 — 对应 C++ svgButton refreshButton
        svgButton 使用 refresh.svg 图标
      -->
      <button class="btn-icon" title="刷新" @click="handleRefresh">
        <img :src="refreshSvg" alt="刷新" class="icon-svg" />
      </button>

      <!-- 弹性空间，将右侧的排序控件推到行尾 -->
      <div class="toolbar-spacer"></div>

      <!--
        排序方式下拉框 — 对应 C++ YComboBox seqWays
        右侧留 margin 40px（参考 C++ row2.removeFromRight(40)）
        宽度 200px，与 C++ row2.removeFromRight(200) 一致
      -->
      <select
        v-model="selectedSort"
        class="combo-sort"
      >
        <option
          v-for="opt in sortOptions"
          :key="opt.value"
          :value="opt.value"
        >
          {{ opt.label }}
        </option>
      </select>

      <!--
        升降序切换 — 对应 C++ upDownButton mUpDownButton
        点击切换 ↑/↓ 图标，与 upDownButton 的 toggle 行为一致
      -->
      <button class="btn-icon" title="切换升降序" @click="toggleSortDirection">
        <img
          :src="isAscending ? upSvg : downSvg"
          :alt="isAscending ? '升序' : '降序'"
          class="icon-svg"
        />
      </button>
    </div>

    <!-- ================================================================
         第 3 块：歌曲列表区域（对应 C++ mViewPort）
         此处是占位区域，后续由 SongSelectViewport 可复用组件替代
         ================================================================ -->
    <div class="viewport-area">
      <!--
        TODO: 放置 <SongSelectViewport /> 组件
        对应 C++ 中的 mSongSelectViewport（SongSelectViewport 类型）
      -->
      <div class="viewport-placeholder">
        <span class="placeholder-text">歌曲列表区域</span>
        <span class="placeholder-hint">（SongSelectViewport 组件待开发）</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
/* ================================================================
   AllMusicPage 容器
   布局与 C++ AllMusicPage::resized() 一致：
   - 顶部标题行 + 工具栏行共占约 20% 高度
   - 底部 viewport 占 80% 高度
   ================================================================ */

.all-music-page {
  display: flex;
  flex-direction: column;
  height: 100%;
  background-color: var(--colorMain);
  user-select: none;
}

/* ================================================================
   第 1 行：标题行 — 对应 C++ row1
   ================================================================ */

.header-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 5px;
  flex-shrink: 0;
}

/* BigLabel: "全部音乐" — fontSize 30（与 C++ BigLabel 一致） */
.page-title {
  font-size: 30px;
  font-weight: bold;
  color: var(--colorTextMain);
  min-width: 90px;  /* 对应 row1.removeFromLeft(90) */
}

/* littleLabel: "共 0 首" — fontSize 15（与 C++ littleLabel 一致） */
.song-count {
  font-size: var(--littleFont);
  color: var(--colorTextSecond);
  min-width: 90px;  /* 对应 row1.removeFromLeft(90) */
}

/* ================================================================
   第 2 行：工具栏行 — 对应 C++ row2
   removeFromLeft(20) → margin-left
   每个控件 reduced(5) → 内缩
   ================================================================ */

.toolbar-row {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 5px;
  padding-left: 20px;  /* 对应 row2.removeFromLeft(20) */
  padding-right: 40px; /* 对应 row2.removeFromRight(40) */
  flex-shrink: 0;
}

/* ── "导入文件" 按钮：对应 yTextButton selectFileButton ── */
.btn-import {
  min-width: 80px;          /* 对应 row2.removeFromLeft(80) */
  height: 30px;             /* 对应 reduced(5,10)：40 - 10 = 30 */
  padding: 0 12px;

  border: none;
  border-radius: var(--borderRadius);
  background-color: var(--colorClicked);
  color: var(--colorTextMain);
  font-size: var(--midFont);
  cursor: pointer;
  transition: background-color var(--easeTime) ease, opacity var(--easeTime) ease;
}

.btn-import:hover {
  background-color: var(--colorHover);
}

.btn-import:active {
  background-color: var(--colorHover);
}

.btn-import:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

/* ── 图标按钮：对应 svgButton / upDownButton ── */
.btn-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 30px;   /* 方形按钮，边长 = toolbar 行高 - 10 */
  height: 30px;

  border: none;
  border-radius: var(--borderRadius);
  background-color: transparent;
  cursor: pointer;
  transition: background-color var(--easeTime) ease;
}

.btn-icon:hover {
  background-color: var(--colorHover);
}

.icon-svg {
  width: 20px;
  height: 20px;
  pointer-events: none;  /* 点击事件由父级 button 处理 */
}

/* ── 弹性空间：将排序控件推到右侧 ── */
.toolbar-spacer {
  flex: 1;
}

/* ── 排序下拉框：对应 YComboBox seqWays ── */
.combo-sort {
  width: 200px;          /* 对应 row2.removeFromRight(200) */
  height: 30px;
  padding: 0 8px;

  border: 3px solid var(--colorEdge);  /* 与 YComboBox kOutlineWidth = 3 一致 */
  border-radius: var(--borderRadius);  /* 与 YComboBox kCornerRadius = 6 一致 */
  background-color: transparent;
  color: var(--colorTextMain);
  font-size: 17px;       /* 与 YComboBox kFontSize = 17 一致 */
  cursor: pointer;
  outline: none;

  transition: border-color var(--easeTime) ease;
}

.combo-sort:hover {
  border-color: var(--colorTextSecond);
}

.combo-sort:focus {
  border-color: var(--colorStress);
}

/* ================================================================
   第 3 块：Viewport 区域 — 对应 C++ mViewPort
   占满剩余高度（约 80%）
   ================================================================ */

.viewport-area {
  flex: 1;
  overflow-y: auto;
  border-top: 2px solid var(--colorEdge);
  position: relative;
}

.viewport-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  gap: 8px;
}

.placeholder-text {
  font-size: var(--bigFont);
  color: var(--colorTextSecond);
}

.placeholder-hint {
  font-size: var(--littleFont);
  color: var(--colorEdge);
}
</style>
