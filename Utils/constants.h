#pragma once
#include <JuceHeader.h>

//此处放置所有模块都需要用到的常量定义
static constexpr int bufferSize{ 1024 };
static constexpr float two_pi{ 2.0f * 3.14159265358979323846f };
static constexpr float pi{ 3.14159265358979323846f };
static constexpr float half_pi{1.57079632679f};
static constexpr float defaultSampleRate{ 44100.0f };
static constexpr float Exp{2.718281828459f};

//attachment别名
using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

//用来序列化的ID
static constexpr const char* TotalAppId{"TotalAppId"};
static constexpr const char* UICacheId{"UICacheId"};
static constexpr const char* AudioArrayId{"AudioArrayId"};//用来存储音频数组,因为数组不能直接交给APVTS管理
static constexpr const char* DIYArrayId{"DIYArrayId"};

