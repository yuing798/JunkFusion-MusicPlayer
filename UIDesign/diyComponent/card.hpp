#pragma once
#include "BinaryData.h"
#include "FontAbout/font.h"
#include "buttons.hpp"
#include "fileMessage.hpp"
#include "juce_animation/juce_animation.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherComponent.hpp"
#include <memory>

class EachStream : public juce::Component{
private:
    PlayStopButton mPlayStopButton;
    YLabel mCountLabel;
    YLabel mNumChannels;
    YLabel mBitRate;
    YLabel mDecoderName;
    YLabel mBitDepth;
    std::unique_ptr<EllipsisToolTip> mEllipsisToolTip;
public:
    EachStream(SongInfo::stream&);
    void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EachStream)
};
class MultiStreamCardButton : juce::TextButton{
    MultiStreamCardButton();
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

