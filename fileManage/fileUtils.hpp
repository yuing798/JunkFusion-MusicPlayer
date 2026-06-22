#pragma once
#include <cstddef>
#include <juce_gui_basics/juce_gui_basics.h>
#include <filesystem>
#include <string>
#include "fileMessage.hpp"

//异步打开文件选择框，支持多选，回调返回选中的文件数组
void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)>,juce::Component* parentComponent = nullptr);

//异步选择文件夹
void getMultiMediaFileDir();

//使用FFmpeg提取元数据
SongInfo getMetaData(std::filesystem::path&);
