#pragma once
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"

// attachment别名
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
using SmoothLinearValue = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;
using SmoothExpValue = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>;

// 日志id
static constexpr const char* LogAudioID{"audio"};
static constexpr const char* LogDllID{"dll"};

// 禁止拷贝和移动的宏
#define DONT_COPY_AND_MOVE(ClassName)                                                              \
    ClassName(const ClassName&) = delete;                                                          \
    ClassName& operator=(const ClassName&) = delete;                                               \
    ClassName(ClassName&&) = delete;                                                               \
    ClassName& operator=(ClassName&&) = delete;
