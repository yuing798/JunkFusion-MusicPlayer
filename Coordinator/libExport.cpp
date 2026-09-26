#include "./libExport.h"
#include "./dllManager.hpp"
#include "Coordinator/libExport.h"
#include "Macro/audioMacro.hpp"
#include "Macro/coordinatorMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "fileManage/dbModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_osc/juce_osc.h"
#include <cstdlib>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

extern "C" {
    void dllInit(const char* cacheDirId, const char* exeDirPtr) {
        // Utils::writeEmergencyLog("准备初始化dll单例1.0");
        dllManager::getInstance().init(cacheDirId, exeDirPtr);
    }
    int toggleMyLike(long long songId) {
        return dllManager::getInstance().getSongsManager().reverseMyLike(songId);
    }
    void saveComment(long long songId, const char* commentText) {
        dllManager::getInstance().getSongsManager().saveComment(commentText, songId);
        return;
    }
    void freeString(char* str) {
        if (str) {
            free(str);
        }
    }
    void someImport(const char* jsonStr) {
        juce::Thread::launch([jsonStr]() {
            auto logger{spdlog::get(LogDllID)};
            // logger->info("接收到的信息为：{}", jsonStr);
            logger->info("开始导入歌曲");
            Yvar obj{juce::JSON::fromString(juce::String::fromUTF8(jsonStr))};
            auto filePaths{obj.read(CoordinatorMacro::filePaths)};
            juce::Array<juce::var> songs;
            juce::Array<juce::var> errorFiles;
            for (int i = 0; i < filePaths.size(); i++) {
                auto path = juce::File(filePaths.read(i).toString());
                auto result = dllManager::getInstance().getSongsManager().insertSong(path);
                if (result.errorMsg.empty()) {
                    songs.add(result.info.toJson());
                    // successFilesString += path.getFileName().toStdString() + "\n";
                } else {
                    auto fileNameStr = path.getFileName();
                    juce::var errorFileObj{new juce::DynamicObject()};

                    errorFileObj.getDynamicObject()->setProperty(
                        CoordinatorMacro::errorFileName,
                        fileNameStr
                    );
                    errorFileObj.getDynamicObject()->setProperty(
                        CoordinatorMacro::errorFileReason,
                        juce::String(result.errorMsg)
                    );
                    errorFiles.add(errorFileObj);
                }
            }
            juce::var resultObj{new juce::DynamicObject()};
            resultObj.getDynamicObject()->setProperty(CoordinatorMacro::songsList, songs);

            resultObj.getDynamicObject()->setProperty(CoordinatorMacro::errorFiles, errorFiles);
            std::string resultStr{
                "导入歌曲完成，成功" + std::to_string(songs.size()) + "首，失败" +
                std::to_string(errorFiles.size()) + "首\n失败文件：\n" +
                juce::JSON::toString(errorFiles).toStdString()
                // + "成功文件" +
                // successFilesString
            };
            logger->info(resultStr);
            if (dllManager::getInstance().onLightSongDataImportOver)
                dllManager::getInstance().onLightSongDataImportOver(
                    ConvertUtils::object2Uint8t(resultObj)
                );
        });
        return;
    }
    void closeBackend() {
        spdlog::get(LogDllID)->debug("准备关闭后端");
        dllManager::destroyInstance();
    }

    void registerErrorSendCallback(StringFunc cb) {
        dllManager::getInstance().onErrorSendCallback = cb;
    }
    void play(long long songId, double currentPTS) {
        auto path = dllManager::getInstance().getSongsManager().getPath(songId);
        juce::var playInfo{new juce::DynamicObject()};

        playInfo.getDynamicObject()->setProperty(AudioMacro::songPath, juce::String(path));
        playInfo.getDynamicObject()->setProperty(AudioMacro::targetPTS, currentPTS);
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(AudioMacro::play, playInfo);
        dllManager::getInstance().sendMessage2AudioProcess(obj);
    }
    void pausePlay() {
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(AudioMacro::pause, "");
        dllManager::getInstance().sendMessage2AudioProcess(obj);
    }
    void registerCurrentPTSCallback(DoubleFunc doubleFunc) {
        dllManager::getInstance().onCurrentPTSCallback = doubleFunc;
    }
    void registerTimeDomainSpecInsertOver(StringFunc cb) {
        dllManager::getInstance().getSongsManager().onTimeDomainSpecInsertOver = cb;
    }
    const char* getTimeDomainSpecBySongId(long long songId) {
        auto vec = dllManager::getInstance().getSongsManager().getTimeDomainSpec(songId);
        juce::Array<juce::var> arr;
        for (auto& i : vec) {
            arr.add(i);
        }
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(CoordinatorMacro::timeDomainSpecs, juce::var(arr));
        auto ptr = ConvertUtils::object2Uint8t(obj);

        return ptr;
    }
    void sendSliderValue(const char* identify, double value, int isOSC) {
        auto identifyParam = juce::String(identify);
        if (isOSC) {
            jassert(identifyParam.startsWith("/"));
            juce::OSCMessage msg(identifyParam, static_cast<float>(value));
            dllManager::getInstance().sendOSCMessage2AudioProcessor(msg);
        } else {
            juce::var obj{new juce::DynamicObject()};

            juce::var paramAndValueObj{new juce::DynamicObject()};
            {
                auto ptr{paramAndValueObj.getDynamicObject()};
                jassert(ptr);
                ptr->setProperty(AudioMacro::sliderParam, identifyParam);
                ptr->setProperty(AudioMacro::sliderValue, value);
            }
            {
                auto ptr{obj.getDynamicObject()};
                jassert(ptr);
                ptr->setProperty(AudioMacro::setSliderValue, paramAndValueObj);
            }
            dllManager::getInstance().sendMessage2AudioProcess(obj);
        }
    }
    void registerOnPlayNextSong(VoidFunc cb) { dllManager::getInstance().onPlayNextSong = cb; }

    void registerOnLightSongDataImportOver(StringFunc cb) {
        dllManager::getInstance().onLightSongDataImportOver = cb;
    }
    // void registerOnGetAllSongsOver(StringFunc cb) {
    //     dllManager::getInstance().onGetAllSongOver = cb;
    // }
    const char* getAllSongs() {
        auto songsList = dllManager::getInstance().getSongsManager().getAllSongs();
        auto obj{new juce::DynamicObject()};
        obj->setProperty(CoordinatorMacro::songsList, songsList);

        return ConvertUtils::object2Uint8t(obj);
    }
}
