#pragma once
#include <cstddef>
#include <juce_gui_basics/juce_gui_basics.h>
#include <filesystem>
#include <string>
#include <vector>
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"

//异步打开文件选择框，支持多选，回调返回选中的文件数组
void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)>,juce::Component* parentComponent = nullptr);

//异步选择文件夹
void getMultiMediaFileDir();

//获取文件的标签信息和解码信息
SongInfo getStreamMetaData(const juce::File&);//因为原本的文件信息中已经包含流路径，所以不需要把路径再传进来


