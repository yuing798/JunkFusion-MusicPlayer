#pragma once

#include "BinaryData.h"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include <cstddef>
#include <vector>
namespace ImageManager {
    
    //将png加载为byteVector,参数二：是否存储图片，图片路径为什么
    std::vector<std::byte> png2MemoryAndFile(juce::Image,const juce::File& path = juce::File{});

    //将jpg加载为byteVector,默认Q值为85,参数三：是否存储图片
    std::vector<std::byte> jpg2MemoryAndFile(juce::Image& , int Q = 85,const juce::File& path = juce::File{});
    juce::Image clipMode(juce::Image& img,int targetWidth = 50,int targetHeight = 50);//裁剪图片

};