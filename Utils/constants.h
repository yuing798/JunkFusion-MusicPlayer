#pragma once
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"

// attachment别名
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

// 日志id
static constexpr const char* LogAudioID{"audio"};
static constexpr const char* LogDllID{"dll"};

// 禁止拷贝和移动的宏
#define DONT_COPY_AND_MOVE(ClassName)                                                              \
    ClassName(const ClassName&) = delete;                                                          \
    ClassName& operator=(const ClassName&) = delete;                                               \
    ClassName(ClassName&&) = delete;                                                               \
    ClassName& operator=(ClassName&&) = delete;
