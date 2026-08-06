// #include "MainComponent.h"
// #include "Utils/BridgeNames.h"
// #include "Utils/otherUtils.hpp"
// #include "constants.h"
// #include "dbModel.hpp"
// #include "fileManage/dbManager.hpp"
// #include "juce_core/juce_core.h"
// #include "juce_events/juce_events.h"
// #include "juce_graphics/juce_graphics.h"
// #include "juce_gui_extra/juce_gui_extra.h"
// #include "serial.hpp"
// #include "songsManage.hpp"
// #include <cstddef>
// #include <cstdint>
// #include <cstring>
// #include <memory>
// #include <optional>
// #include <spdlog/spdlog.h>
// #include <string>
// #include <utility>
// #include <vector>

// //==============================================================================
// MainComponent::MainComponent() {

//     // 这里定义的都是桥接通信函数
//     //     juce::WebBrowserComponent::Options options;
//     //     options =
//     //         options.withBackend(juce::WebBrowserComponent::Options::Backend::webview2)

//     //             // 下面放置的是后端需要直接和前端交互的函数
//     //             .withResourceProvider(
//     //                 [this](const juce::String& path)
//     //                     -> std::optional<juce::WebBrowserComponent::Resource> {
//     //                     // 去掉开头的 "/"，与 JUCE 官方 WebViewPluginDemo 的
//     //                     fromFirstOccurrenceOf
//     //                     // 处理方式一致
//     //                     const auto cleanPath = path.fromFirstOccurrenceOf("/", false, false);
//     //                     juce::StringArray tokens;
//     //                     tokens.addTokens(cleanPath, "/", ""); // 将原始URL按照斜杠进行切分
//     //                     if (tokens[0] == "songId") {
//     //                         int64_t songId{tokens[1].getLargeIntValue()};
//     //                         if (tokens[2] ==
//     //                             "image") { //
//     //                             歌曲的信息,URL格式为/songId/8175019024(id号)/image/imageType
//     //                             std::string hash{
//     // dbManager::getInstance().getSongsManager().getImageHashBySongId(
//     //                                     songId)};
//     //                             juce::File songImageDir{songImageDirId.getChildFile(hash)};

//     //                             if (tokens[3] == "50x50") {
//     //                                 if (hash.empty()) {
//     //                                     return juce::WebBrowserComponent::Resource(
//     //                                         imageHolder50x50,
//     //                                         "image/png"); // 没有图片的话返回占位图片
//     //                                 } else {

//     //                                     juce::File
//     //                                     image50x50{songImageDir.getChildFile("50x50.jpg")}; if
//     //                                     (!image50x50
//     //                                              .existsAsFile()) { //
//     //                                              说明是第一次加载50x50图片，没有加入缓存
//     //                                         juce::Image originalImage{
//     // juce::ImageCache::getFromFile(songImageDir.getChildFile(
//     //                                                 "original.jpg"))}; // 得到原始图片

//     //                                         // 裁剪图片
//     //                                         juce::Image songImage50x50 =
//     //                                             ImageManager::clipMode(originalImage, 50, 50);

//     //                                         // 加载到磁盘和内存中
//     //                                         auto vec =
//     //                                         ImageManager::jpg2MemoryAndFile(songImage50x50,
//     //                                                                                    70,
//     // image50x50);
//     //                                         return juce::WebBrowserComponent::Resource(vec,
//     // "image/jpeg");
//     //                                     } else {
//     //                                         return juce::WebBrowserComponent::Resource(
//     //                                             loadFile2ByteVector(image50x50),
//     "image/jpeg");
//     //                                     }
//     //                                 }
//     //                             } else if (tokens[3] == "240x240") {
//     //                                 if (hash.empty()) {
//     //                                     return juce::WebBrowserComponent::Resource(
//     //                                         imageHolder240x240,
//     //                                         "image/png"); // 没有图片的话返回占位图片
//     //                                 } else {

//     //                                     juce::File image240x240{
//     //                                         songImageDir.getChildFile("240x240.jpg")};
//     //                                     if (!image240x240
//     //                                              .existsAsFile()) { //
//     //                                              说明是第一次加载240x240图片，没有加入缓存
//     //                                         juce::Image originalImage{
//     // juce::ImageCache::getFromFile(songImageDir.getChildFile(
//     //                                                 "original.jpg"))}; // 得到原始图片

//     //                                         // 裁剪图片
//     //                                         juce::Image songImage240x240 =
//     //                                             ImageManager::clipMode(originalImage, 240,
//     240);

//     //                                         // 加载到磁盘和内存中
//     //                                         auto vec = ImageManager::jpg2MemoryAndFile(
//     //                                             songImage240x240, 80, image240x240);
//     //                                         return juce::WebBrowserComponent::Resource(vec,
//     // "image/jpeg");
//     //                                     } else {
//     //                                         return juce::WebBrowserComponent::Resource(
//     //                                             loadFile2ByteVector(image240x240),
//     "image/jpeg");
//     //                                     }
//     //                                 }
//     //                             }
//     //                         }
//     //                     }
//     //                     return std::nullopt;
//     //                 }
