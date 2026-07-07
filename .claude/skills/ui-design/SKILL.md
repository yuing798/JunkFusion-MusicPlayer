---
name: ui-design
description: This document explains how to use Vue for UI design.
---

1. 在UI设计的时候，请不要使用emoji，需要的符号请告诉我，我去网站上面给你找
2. UI/src/assets文件夹中放置了图片和定义好了的css布局，请优先使用这些资源进行设计
3. 在用户让你完成一个完整文件的前提下，请在这个你完成的文件的最上方写出完整的接口示例，如

```vue
// ════════════════════════════════════════════════════════════════
// Tooltip — 鼠标悬停提示窗
//
// 使用方式：
//   <Tooltip text="这是提示文本">
//     <button>悬停我</button>
//   </Tooltip>
//
//   <Tooltip :text="dynamicText">
//     <SomeComponent />
//   </Tooltip>
//
// 特性：
//   - 鼠标悬停 300ms 后出现
//   - 位置跟随鼠标，自动避让屏幕边界
//   - 通过 Teleport 渲染到 body，不受应用窗口裁剪
// ════════════════════════════════════════════════════════════════
```