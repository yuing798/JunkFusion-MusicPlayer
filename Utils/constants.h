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

static const juce::File LocalDirId{
    juce::File::getSpecialLocation(
        #ifdef WIN32
        juce::File::SpecialLocationType::windowsLocalAppData
        #else
        juce::File::SpecialLocationType::userApplicationDataDirectory
        #endif
    ).getChildFile("JunkFusion")
};
static const juce::File imageDirId{LocalDirId.getChildFile("image")};
static const juce::File databaseDirId{LocalDirId.getChildFile("database")};
static const juce::File logInfoDirId{LocalDirId.getChildFile("logInfo")};

//禁止拷贝和移动的宏
#define DONT_COPY_AND_MOVE(ClassName) \
ClassName(const ClassName&) = delete; \
ClassName& operator=(const ClassName&) = delete; \
ClassName(ClassName&&) = delete; \
ClassName& operator=(ClassName&&) = delete;

