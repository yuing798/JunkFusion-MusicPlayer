# Junk-Fusion音乐播放器项目

这是一个以flutter做UI,juce做为音频引擎，FFmpeg做编码层，onnx runtime做AI推理层，spdlog做日志分析的音乐播放器项目，flutter作为前端UI
主要目标为覆盖三桌面平台:macos,windows,linux

## 软件工程规范

1. 完成任务后不要自行build，因为沙盒限制你是不会编译通过的
2. 进行架构调整的时候建议看一下我的git记录
3. 我没叫你做的事情决定禁止做，你可以向我提议，但是禁止亲自做
4. 所有日志放在C:\Users\sakuyayuing\AppData\Local\Junk Fusion\Junk Fusion\log文件夹中，需要时自行查阅
5. 你在写代码的时候，遇到不确定的内容请使用TODO，FIXME标识先待定，之后我会亲自补充
6. 当你完成一个全新的文件的时候，请在文件的开头写明这个文件的内容概述，API调用方法，文件的意义，如

``` text
/**
 * @brief 可选链(optional chaining)风格的安全 JSON 读写
 *
 * ── 设计目的只有两个 ──────────────────────────────────────────────────
 * 1. 安全读: 取不到值不抛异常、也不返回"必须判断的东西"，而是返回**空值**，
 *    可以像 JS 的 `?.` 一样一路链下去:
 *
 *        auto name = v.read("results").read(0).read("artists").read(0).read("name").toString();
 *        // 中间任何一环缺失 -> name == ""，一个 if 都不用写
 *
 * 2. 安全写: set/push 的写入类型被白名单(YvarDetail::isSafeValue)约束，
 *    nlohmann 那套"什么都能隐式转换、编译期全过、运行时才炸"的坑
 *    (char 存成 97、枚举退化成整数、空指针写进 json、越界写)
 *    全部变成编译错误。
 *
 * ── 取值三段式 ────────────────────────────────────────────────────────
 * | 写法        | 缺失 / 类型不符时          | 用在哪                       |
 * |-------------|----------------------------|------------------------------|
 * | read 链     | 返回空 Yvar，链继续        | 中间层导航、第三方 API       |
 * | toXxx()     | 安全默认值(""、0、false)   | 可以缺省的字段               |
 * | getOpt<T>() | std::nullopt               | 末尾一条 if 就能处理         |
 * | get<T>()    | 抛 Yvar::TypeError         | 必需字段、内部协议、数据损坏 |
 *
 * get<T>() 只在**真正必须存在**的字段上用，不要拿它读第三方 API 的稀疏字段。
 */
class Yvar {
```

7. 当我的要求出现你感觉非常不合理的想法的时候，请立刻停下来说明你觉得不合理的地方并反问我是否确定要这么做，然后我确定了再开工.
8. 禁止在cpp文件顶层使用namespace或者static定义临时工具函数，需要工具函数请放在私有成员或者Utils/文件夹中
9. 当前项目处于开发阶段，不需要有兼容旧版本的操作，特别是数据库表部分


## 音频开发规范

1. 在音频线程进行耗时操作只可能造成音频卡顿，不可能出现内存越界等安全问题，也不可能出现DSP效果完全消失的问题

## 代码架构

Utils/:
    Yvar:放置json安全读写类，其他文件读写json时请使用Yvar,不要使用nlohmann::json
    DbUtils:数据库工具类
    ConvertUtils:类型转换工具
    MathUtils:数学工具(通常只在音频DSP中会使用到)
    StringUtils:字符串工具
    OtherUtils:其他乱七八糟不好分类的工具都放到这里面
Models/:CPP模型类
Macro/:CPP使用的宏
junk_fusion_ui/:flutter前端
    lib/:
        utils/:前端工具类
        widgets/:前端UI组件
            helper_widget/:辅助组件
            pages/:页面
            playControl:/播放控制组件
            popupWindow/:弹窗组件
        bridge/:和后端桥接的工具类
        model/:模型类，统一使用freezed生成
DevScripts/:开发者工具
Coordinator:协调者，用来数据库读写和协调多模块
AudioProcessor:音频进程：只能由协调者通信，前端不可直接与其通信