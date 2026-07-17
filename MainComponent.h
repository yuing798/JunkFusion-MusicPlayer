#pragma once
#include "BinaryData.h"
#include "fileManage/songsManage.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include <cstddef>
#include <cstdint>
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <vector>
#include "image/ImageManager.hpp"
#include "Utils/LRUCachePool.hpp"

class MainComponent final : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    void resized() override;
    void paint(juce::Graphics& g) override;

private:

    std::unique_ptr<juce::WebBrowserComponent> web;
    songsManageBuilder mSongsManagerBuilder;

    std::vector<std::byte> imageHolder50x50{ImageManager::png2MemoryAndFile(juce::ImageCache::getFromMemory(
        BinaryData::JunkFusion_png,
        BinaryData::JunkFusion_pngSize
    ).rescaled(50,50))};//没有歌曲图片的时候的占位符
    //临时对象只能绑定到 const 左值引用或右值引用

    // LRUCachePool<int64_t,std::vector<std::byte>> image50x50CachePool{1000};
    // //歌曲封面直接使用songId来缓存，因为占用内存小，假设一张5050的图片为6KB，1000张也才6MB

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
