#include "fileUtils.hpp"
#include "juce_core/juce_core.h"
#include "portable-file-dialogs.h"
#include <memory>
#include <thread>
#include <vector>

const std::vector<std::string> fileTypeArray{
    "All Supported Media",
    "*.wav *.mp3 *.flac *.aiff *.aac *.m4a *.ogg *.wma *.opus *.mp4 *.mkv *.mov *.avi *.wmv *.flv *.webm",

    // ====== 🎵 常见音频格式 ======
    "MP3 Audio", "*.mp3",
    "WAV Audio", "*.wav",
    "FLAC (Lossless)", "*.flac",
    "M4A Audio", "*.m4a",
    "AAC Audio", "*.aac",
    "OGG Audio", "*.ogg",
    "WMA Audio", "*.wma",
    "AIFF Audio", "*.aiff",
    "OPUS Audio", "*.opus",

    // ====== 🎬 常见视频格式 (用于提取音频) ======
    "MP4 Video", "*.mp4",
    "MKV Video", "*.mkv",
    "MOV (Apple)", "*.mov",
    "AVI Video", "*.avi",
    "WMV Video", "*.wmv",
    "FLV (Flash)", "*.flv",
    "WEBM Video", "*.webm",
    "all file","*"
};


void showMediaFileChooser(std::function<void(const juce::File&)> onFileSelected,juce::Component* parentComponent){

    std::vector<std::unique_ptr<juce::WildcardFileFilter>> filters;

}