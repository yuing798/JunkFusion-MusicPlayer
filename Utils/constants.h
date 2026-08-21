#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"

// attachment别名
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

// 日志id
static constexpr const char* LogAudioID{"audio"};
static constexpr const char* LogSchedulerID{"scheduler"};
static constexpr const char* LogDllID{"dll"};
static constexpr const char* LogAiID{"ai"};
static constexpr const char* LogVSTID{"vst"};
static constexpr const char* LogCrashID{"crash"};
static constexpr const char* LogAllID{"all"};

namespace defsStr {
    static constexpr const char* PopupWindowType{"PopupWindowType"};
    static constexpr const char* infoMsg{"info"};
    static constexpr const char* errorMsg{"error"};
    static constexpr const char* msg{"msg"};
} // namespace defsStr

// 禁止拷贝和移动的宏
#define DONT_COPY_AND_MOVE(ClassName)                                                              \
    ClassName(const ClassName&) = delete;                                                          \
    ClassName& operator=(const ClassName&) = delete;                                               \
    ClassName(ClassName&&) = delete;                                                               \
    ClassName& operator=(ClassName&&) = delete;
