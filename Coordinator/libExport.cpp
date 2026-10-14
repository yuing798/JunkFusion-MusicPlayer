#include "./libExport.h"
#include "./dllManager.hpp"
#include "Coordinator/libExport.h"
#include "Macro/SongInfoMacro.hpp"
#include "Macro/audioMacro.hpp"
#include "Macro/coordinatorMacro.hpp"
#include "Model/SongInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_osc/juce_osc.h"
#include <cstdlib>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <variant>
#include <vector>

extern "C" {
    void dllInit(const char* cacheDirId, const char* exeDirPtr) {
        OtherUtils::writeEmergencyLog("准备初始化dll单例");
        dllManager::getInstance().init(cacheDirId, exeDirPtr);
    }
    void toggleMyLike(long long songId) {
        dllManager::getInstance().getDbContext()->songsManage.reverseMyLike(songId);
    }
    void freeString(char* str) {
        if (str) {
            free(str);
        }
    }
    void someImport(const char* jsonStr) {
        auto jsonString = std::string(jsonStr);
        juce::Thread::launch([jsonStr = std::move(jsonString)]() {
            auto logger{spdlog::get(LogDllID)};
            logger->info("开始导入歌曲");
            logger->debug("jsonStr == {}", jsonStr);
            Yvar obj{juce::JSON::fromString(jsonStr)};
            auto filePaths{obj.read(CoordinatorMacro::filePaths)};
            std::vector<juce::File> paths;
            for (int i = 0; i < filePaths.size(); i++) {
                paths.push_back(juce::File{filePaths.read(i).toString()});
            }
            juce::Array<juce::var> errorFiles;
            juce::Array<juce::var> songInfos;
            for (auto& path : paths) {
                auto result = dllManager::getInstance().insertSong(path);
                if (std::holds_alternative<std::string>(result)) {
                    auto errorMsg = std::get<std::string>(result);
                    juce::var errorObj{new juce::DynamicObject()};
                    errorObj.getDynamicObject()->setProperty(
                        CoordinatorMacro::errorFileName,
                        path.getFileName()
                    );
                    errorObj.getDynamicObject()->setProperty(
                        CoordinatorMacro::errorFileReason,
                        juce::String{errorMsg}
                    );
                    errorFiles.add(errorObj);
                } else if (std::holds_alternative<SongInfo>(result)) {
                    auto songInfo = std::get<SongInfo>(result);
                    songInfos.add(songInfo.toBasicJson());
                }
            }

            juce::var resultObj{new juce::DynamicObject()};
            {

                resultObj.getDynamicObject()->setProperty(CoordinatorMacro::errorFiles, errorFiles);

                std::string resultStr{
                    "导入歌曲完成，成功" + std::to_string(paths.size()) + "首，失败" +
                    std::to_string(errorFiles.size()) + "首\n失败文件：\n" +
                    juce::JSON::toString(errorFiles).toStdString()
                };
                logger->info(resultStr);
            }
            resultObj.getDynamicObject()->setProperty(CoordinatorMacro::songsList, songInfos);

            // 发送最终数据
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
    void play(long long songId, double targetPTS) {
        auto info =
            dllManager::getInstance().getDbContext()->songsManage.getPlayInfo(songId).toJson();
        juce::var playObj{new juce::DynamicObject()};
        {
            playObj.getDynamicObject()->setProperty(AudioMacro::playInfo, info);
            playObj.getDynamicObject()->setProperty(AudioMacro::targetPTS, targetPTS);
        }
        juce::var resultObj{new juce::DynamicObject()};
        resultObj.getDynamicObject()->setProperty(AudioMacro::play, playObj);
        dllManager::getInstance().sendMessage2AudioProcess(resultObj);
    }
    void pausePlay() {
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(AudioMacro::pause, "");
        dllManager::getInstance().sendMessage2AudioProcess(obj);
    }
    void registerCurrentPTSCallback(DoubleFunc doubleFunc) {
        dllManager::getInstance().onCurrentPTSCallback = doubleFunc;
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
    void registerOnPlayNextOrPreviousSong(IntFunc cb) {
        dllManager::getInstance().onPlayNextOrPreviousSong = cb;
    }

    void registerOnLightSongDataImportOver(StringFunc cb) {
        dllManager::getInstance().onLightSongDataImportOver = cb;
    }
    void registerOnUpdateSongInfo(Int64Func cb) { dllManager::getInstance().onUpdateSongInfo = cb; }
    const char* getAllSongs() {
        auto songsList =
            dllManager::getInstance().getDbContext()->songsManage.getAllSongsBasicInfo();
        auto obj{new juce::DynamicObject()};
        obj->setProperty(CoordinatorMacro::songsList, songsList);

        return ConvertUtils::object2Uint8t(obj);
    }
    const char* getCompleteSongInfo(long long songId) {
        auto info{
            dllManager::getInstance().getDbContext()->songsManage.getCompleteSongInfo(songId)
        };
        return ConvertUtils::object2Uint8t(info.toCompleteJson());
    }
    void requestOnPlayStateSync(IntFunc cb) { dllManager::getInstance().onPlayStateSync = cb; }

    const char* getAllAlbumView() {
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(
            CoordinatorMacro::albumsList,
            dllManager::getInstance().getDbContext()->albumsManage.getAllAlbumsView()
        );
        return ConvertUtils::object2Uint8t(obj);
    }

    const char* getCompleteAlbumInfo(long long albumId) {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance().getDbContext()->albumsManage.getCompleteInfo(albumId)
        );
    }
}
