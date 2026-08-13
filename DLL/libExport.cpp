#include "./libExport.h"
#include "./dllManager.hpp"
#include "constants.h"
#include "dllBridgeName.hpp"
#include "dllUtils.hpp"
#include "fileManage/dbModel.hpp"
#include "juce_core/juce_core.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

extern "C" {
    void dllInit(const char* cacheDirId, const char* exeDirPtr) {
        dllManager::getInstance().init(cacheDirId, exeDirPtr);
    }
    int getAllSongCount() {
        auto count{dllManager::getInstance().getSongsManager().getTotalSongCount()};
        if (count.has_value()) {
            return count.value();
        } else {
            return 0;
        }
    }
    int toggleMyLike(long long songId) {
        return dllManager::getInstance().getSongsManager().reverseMyLike(songId);
    }
    const char* getAllSongs() {
        std::vector<SongInfo> songs = dllManager::getInstance().getSongsManager().getAllSongs();
        auto obj{new juce::DynamicObject()};
        obj->setProperty(B_getAllSongs::songsList, juce::var(SongInfo::vector2VarArray(songs)));
        return object2Uint8t(obj);
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
    const char* someImport(const char* jsonStr) {
        auto logger{spdlog::get(LogDllID)};
        // logger->info("接收到的信息为：{}", jsonStr);
        logger->info("开始导入歌曲");
        auto obj = charPtr2object(jsonStr);
        auto filePaths = obj->getProperty(B_songImport::filePaths).getArray();
        std::vector<SongInfo> songs;
        juce::Array<juce::var> errorFiles;
        std::string errorFilesString;
        // std::string successFilesString;
        for (auto& filePath : *filePaths) {
            auto path = juce::File(filePath.toString());
            auto result = dllManager::getInstance().getSongsManager().insertSong(path);
            if (result.has_value()) {
                songs.push_back(result.value());
                // successFilesString += path.getFileName().toStdString() + "\n";
            } else {
                auto pathStr = path.getFileName();
                errorFiles.add(pathStr);
                errorFilesString += path.getFileName().toStdString() + "\n";
            }
        }
        juce::DynamicObject::Ptr resultObj{new juce::DynamicObject()};
        resultObj->setProperty(B_songImport::songs, SongInfo::vector2VarArray(songs));
        resultObj->setProperty(B_songImport::errorFiles, errorFiles);
        std::string resultStr{
            "导入歌曲完成，成功" + std::to_string(songs.size()) + "首，失败" +
            std::to_string(errorFiles.size()) + "首\n失败文件：\n" + errorFilesString
            // + "成功文件" +
            // successFilesString
        };
        // 这里准备加上失败原因
        logger->info(resultStr);
        return object2Uint8t(resultObj);
    }
    // void RegisterExitCallback(BackendExitCallback callback) { g_onExitCallback = callback; }
    // bool StartBackendProcess(const char* backendPath, const char* cacheDir) {
    //     std::lock_guard<std::mutex> lock(g_state.mtx);
    //     // 如果已经在运行，则直接返回成功（幂等）
    //     if (g_state.running) return true;

    //     std::string path(backendPath ? backendPath : "");
    //     std::string cache(cacheDir ? cacheDir : "");
    //     if (path.empty() || cache.empty()) return false;

    //     bool ok = PlatformStartProcess(path, cache);
    //     if (ok) {
    //         g_state.running = true;
    //     }
    //     return ok;
    // }
    // void StopBackendProcess() { PlatformStopProcess(); }
}
