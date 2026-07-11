#pragma once
#include "fileManage/songsManage.hpp"
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
