---
name: cpp-style
description: This document explains how you should write C++ code.
---

1. 对于短且可复用的字符串，需要在Utils/constants.h中先定义再使用
如：
```cpp
static constexpr const char* LogCrashID{"crash"};
```
2. 统一称呼
在我的提问和你的回答中
```cpp
struct classB{
    ......
}
struct classA{
    classB mClassB;
}//这种统一称呼为包含类与被包含类

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

5. 类方法在头文件中声明,类方法在cpp文件中定义，除非函数内容很短或者使用嵌套结构体
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

6. 数据库增删查改的字符串(如bind函数)不要直接写数字，改成:字符串的形式,如
```cpp
stmt.bind(1,  info.filePath);
//改成命名参数，如
std::string sql = 
    "INSERT INTO songs (filePath, fileSize, lastModifiedTime, isMultiStreamFile, duration) "
    "VALUES (:filePath, :fileSize, :lastModifiedTime, :isMultiStreamFile, :duration);";

stmt.bind(":filePath", info.filePath);
stmt.bind(":fileSize", info.fileSize);
stmt.bind(":lastModifiedTime", info.lastModifiedTime);
stmt.bind(":isMultiStreamFile", info.isMultiStreamFile);
stmt.bind(":duration", info.duration);

info.filePath = query.getColumn("filePath").getString();
info.fileSize = query.getColumn("fileSize").getInt64();
```
