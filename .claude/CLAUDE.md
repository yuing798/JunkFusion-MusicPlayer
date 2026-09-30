# Junk-Fusion音乐播放器项目

这是一个以flutter做UI,juce做为音频引擎，FFmpeg做编码层，onnx runtime做AI推理层，spdlog做日志分析的音乐播放器项目，flutter作为前端UI
主要目标为覆盖三桌面平台:macos,windows,linux

## 软件工程规范

1. 完成任务后不要自行build
2. 进行架构调整的时候建议看一下我的git记录
3. 我没叫你做的事情决定禁止做，你可以向我提议，但是禁止亲自做
4. 业务bug和性能bug(应用崩溃，空指针，白屏)的危险等级同样高，禁止为了解决性能bug引入业务bug
5. 所有日志放在C:\Users\sakuyayuing\AppData\Local\Junk Fusion\Junk Fusion\log文件夹中，需要时自行查阅
6. 你在写代码的时候，遇到不确定的内容请使用TODO，FIXME标识先待定，之后我会亲自补充