#pragma once
#include "juce_core/juce_core.h"
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

//日志id
static constexpr const char* LogAudioID{"audio"};
static constexpr const char* LogSchedulerID{"scheduler"};
static constexpr const char* LogUiID{"ui"};
static constexpr const char* LogAiID{"ai"};
static constexpr const char* LogVSTID{"vst"};
static constexpr const char* LogCrashID{"crash"};
static constexpr const char* LogAllID{"all"};

//文件路径操作

static const juce::File UserDirId{
    juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("user")};
static const juce::File imageDirId{UserDirId.getChildFile("image")};
static const juce::File databaseDirId{UserDirId.getChildFile("database")};
static const juce::File logInfoDirId{UserDirId.getChildFile("logInfo")};

