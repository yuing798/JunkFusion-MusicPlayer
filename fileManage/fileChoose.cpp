#include "fileChoose.hpp"
#include "FontAbout/font.h"
#include "UISet.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

const std::vector<std::string> fileTypeArray{
    "All Supported Media","*",
    "*.wav *.mp3 *.flac *.aiff *.aac *.m4a *.ogg *.wma *.opus *.mp4 *.mkv *.mov *.avi *.wmv *.flv *.webm",

    // ====== 🎵 常见音频格式 ======
    "MP3 Audio","*", "*.mp3",
    "WAV Audio","*", "*.wav",
    "FLAC (Lossless)","*", "*.flac",
    "M4A Audio","*", "*.m4a",
    "AAC Audio","*", "*.aac",
    "OGG Audio","*", "*.ogg",
    "WMA Audio","*", "*.wma",
    "AIFF Audio","*", "*.aiff",
    "OPUS Audio","*", "*.opus",

    // ====== 🎬 常见视频格式 (用于提取音频) ======
    "MP4 Video","*", "*.mp4",
    "MKV Video","*", "*.mkv",
    "MOV (Apple)","*", "*.mov",
    "AVI Video","*", "*.avi",
    "WMV Video","*", "*.wmv",
    "FLV (Flash)","*", "*.flv",
    "WEBM Video","*", "*.webm",
    "all file","*","*"
};


void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)> onFileSelected,
                          juce::Component* parentComponent)
{

    // 1. 构建过滤器字符串（用分号分隔）
    juce::String filters = 
        "*.wav;*.mp3;*.flac;*.aiff;*.aac;*.m4a;*.ogg;*.wma;*.opus;"
        "*.mp4;*.mkv;*.mov;*.avi;*.wmv;*.flv;*.webm";

    // 2. 创建 FileChooser 对象（使用 shared_ptr 管理生命周期）
    auto chooser = std::make_shared<juce::FileChooser>(
        "请选择多媒体文件（音频或视频）",                // 对话框标题
        juce::File::getSpecialLocation(juce::File::userHomeDirectory), // 初始目录
        filters,                                         // 过滤器字符串
        true,                                            // 使用原生对话框（外观更好）
        false,                                           // 不将包视为目录
        parentComponent                                  // 父组件（实现模态）
    );

    // 3. 异步启动对话框
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems, // 支持多选
        [chooser, onFileSelected](const juce::FileChooser&)//回调函数的传参只有在回调实际触发时才能知道
            
            // std::function：这是一个通用的函数包装器，意味着你可以传入任何可调用的对象：
            // 普通函数、函数指针、Lambda 表达式、std::bind 生成的对象等。

            // void ( ... )：这个回调函数没有返回值。

            // const FileChooser &：回调被触发时，系统会把启动这次操作的 FileChooser 对象的常量引用传进来。
            
            {
                // 获取选中的所有文件
                juce::Array<juce::File> selected = chooser->getResults();

                // 调用回调，传递文件列表
                if (onFileSelected){//这个地方是在检查 std::function 这个“对象”是否为空
                    onFileSelected(selected);
                }
            }
    );
}