#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

//异步打开文件选择框，支持多选，回调返回选中的文件数组
void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)>,
                          juce::Component* parentComponent = nullptr);

//获取某个文件夹下面的所有合适的多媒体文件
void getMultiMediaFileDir();