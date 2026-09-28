# JunkFusion音乐播放器项目的UI层

## 可用依赖

flutter:
  sdk: flutter
freezed_annotation: ^3.1.0 # 运行时需要用到的注解
json_annotation: ^4.12.0 # 用于 JSON 的注解
tabler_icons_plus: ^3.44.0 #增强版图标
shared_preferences: ^2.5.0 #和localStorage一样，做轻量数据存储
ffi: ^2.2.0 #和cpp的通信接口
file_picker: ^11.0.3 #文件选择
smooth_scroll_multiplatform: ^ 1.0.8 #可平滑操作但不能键盘滚动的滚动条
package_rename_plus: ^1.11.0 #包重命名
provider: ^6.1.2
window_manager: ^0.5.2 #窗口管理
logger: ^2.7.0 #日志功能
path_provider: ^2.1.6 #提供计算机特殊文件夹路径
tray_manager: ^0.5.3 #托盘管理
audio_video_progress_bar: ^2.0.3 #进度条,
collection: ^1.19.1 #提供firstWhereOrNull
windows_taskbar: ^1.1.2 #windows专用任务栏管理
win32: ^5.9.0 #windows提供任务栏窗口缩略图

## 代码架构

utils/:工具类和工具函数
bridge/:桥接类
Macro/:你可以使用的宏定义(如果开头有warning:this file will be generated auto,dont modify it by yourself的话，禁止私自修改文件内容)，宏定义请都放在该文件夹中
model/:数据模型
pages/:主页面
theme/:主题
widgets/:组件

## 代码规范

1. 在阅读依赖包文件的时候，只需要阅读接口如何调用即可，禁止深入阅读其他文件
2. 调试语句直接使用print函数，因为我的flutter前端基本只用于绘图，没有多少业务逻辑，不需要写入日志