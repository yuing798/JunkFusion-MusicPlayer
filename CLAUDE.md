# Junk-Fusion音乐播放器项目

这是一个以juce做UI和音频引擎，FFmpeg做编码层，onnx runtime做AI推理层，spdlog做日志分析的音乐播放器项目

## 代码规范

1.所有命名统一采用驼峰式，包括cpp,sqlite,python语句

### cpp规范

1. 类方法在头文件中声明,类方法在cpp文件中定义，除非函数内容很短或者使用嵌套结构体
如：
```cpp
struct myClass{
    void processBlock(juce::AudioBuffer& buffer);

    int c;
    
    struct subStruct{
        int a,b;
        int compute(){
            a *=10;
            b -=3;
            return a*b;
        }//嵌套结构体中的方法直接在头文件定义就可以了
    }
    int& getC(){return c;}//只有一行的方法直接在头文件定义就行了
}
```
2. 完成任务后不要自行build
3. 临时工具函数请使用lambda的形式，不要在外部在加上一个namespace或者static来定义，如
```cpp
namespace
{
    int safeToInt(const char* str)
    {
        try { return std::stoi(str); }
        catch (...) { return 0; }
    }
}//这种写法是错的，应该写成下面这种形式

auto safeToInt = [](const char* str) -> int{
    try { return std::stoi(str); }
    catch (...) { return 0; }
};//写成这种形式，然后在同一个函数定义体中使用
```
4. 禁止使用namespace
5. 进行架构调整的时候建议看一下我的git记录


