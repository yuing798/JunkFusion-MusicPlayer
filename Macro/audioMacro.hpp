#pragma once
namespace AudioMacro {

    static constexpr const char* cacheDir = "--cacheDir";
    static constexpr const char* pushPullPort = "--pushPullPort";
    static constexpr const char* pubSubPort = "--pubSubPort";
    static constexpr const char* deviceTypeList = "deviceTypeList";
    static constexpr const char* deviceList = "deviceList";
    static constexpr const char* sampleRateList = "sampleRateList";
    static constexpr const char* bufferSizeList = "bufferSizeList";
    static constexpr const char* oscPort = "--oscPort";
    static constexpr const char* errorPopupWindowMsg = "errorPopupWindowMsg";
    static constexpr const char* killAudioProcess = "killAudioProcess";
    static constexpr const char* pause = "pause";
    static constexpr const char* songChangeCrossFadeLength = "songChangeCrossFadeLength";
    static constexpr const char* play = "play";
    static constexpr const char* playInfo = "playInfo";
    static constexpr const char* targetPTS = "targetPTS"; // 目标进度条时间戳

    // 这个是从音频进程向UI发送的，上面那个反过来
    static constexpr const char* currentPTS = "currentPTS";
    static constexpr const char* setSliderValue = "setSliderValue";
    static constexpr const char* sliderParam = "sliderParam";
    static constexpr const char* sliderValue = "sliderValue";
    static constexpr const char* onPlayNextOrPreviousSong = "onPlayNextOrPreviousSong";
    static constexpr const char* updatePlayCount = "updatePlayCount"; // 请求更新播放次数
    // 请求播放状态同步,比如smtc切换播放状态就是直接后端通知前端的
    static constexpr const char* playStateSync = "playStateSync";
} // namespace AudioMacro