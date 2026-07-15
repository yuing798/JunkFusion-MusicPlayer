#pragma once
#include "BinaryData.h"
#include "fileManage/songsManage.hpp"
#include "juce_graphics/juce_graphics.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>

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

    juce::Image imageHolder50x50{juce::ImageCache::getFromMemory(
        BinaryData::JunkFusion_png,
        BinaryData::JunkFusion_pngSize
    ).rescaled(50,50)};//没有歌曲图片的时候的占位符

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
