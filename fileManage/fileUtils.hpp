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

//使用FFmpeg提取元数据
SongInfo getMetaData(std::filesystem::path&);

//获取单个文件的信息接口，因为单个文件可能有多条流，所以需要在套一层外层接口,但是文件层面的信息多条流之间是可以共享的
SongInfo getFileMetaData(const juce::File&);

//获取单条流的标签信息和解码信息
std::vector<SongInfo> getStreamMetaData(const SongInfo&);//因为原本的文件信息中已经包含流路径，所以不需要把路径再传进来


