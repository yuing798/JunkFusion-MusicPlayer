#pragma once

#include "UISet.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

class LeftSelectedComponent : public juce::Component{
//这个东西就是主页面左边可以用来切换页面的侧边框，选择我喜欢的音乐，作者，那些乱七八糟的东西
private:
    juce::Viewport mViewPort;//滑动窗口
    std::vector<transparentButton> buttons;
public:
    LeftSelectedComponent();
};
