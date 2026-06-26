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
6. 对于短且可复用的字符串，需要在Utils/constants.h中先定义再使用
如：
```cpp
static constexpr const char* LogCrashID{"crash"};
```
7. 统一称呼
在我的提问和你的回答中
```cpp
struct classB{
    ......
}
struct classA{
    classB mClassB;
}//这种统一称呼为父类和子类

struct classB{
    ......
}
struct classA : juce::Component{
    juce::Component mClassB;
    classA(){
        addAndMakeVisible(mClassB);
    }
}//这种统一叫作父组件和子组件

struct classA : classB{

}//这种统一叫作基类和继承类
```
这条规范很重要，请不要弄错名称

8. 在UI设计的时候，请不要使用emoji，需要的符号请告诉我，我去lucide.dev网站上面给你找
9. 我没叫你做的事情决定禁止做，你可以向我提议，但是禁止亲自做
10. 不要把我写的注释给删了，就算那段注释看起来没有用


