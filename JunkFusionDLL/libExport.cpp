#include "./libExport.h"
#include "./dllManager.hpp"
#include "constants.h"
#include "dllBridgeName.hpp"
#include "dllUtils.hpp"
#include "fileManage/dbModel.hpp"
#include "juce_core/juce_core.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <vector>

extern "C" {
    void dllInit(const char* cacheDirId) { dllManager::getInstance().init(cacheDirId); }
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
        logger->info("开始导入歌曲");
        auto obj = charPtr2object(jsonStr);
        juce::Array<juce::var> filePaths = obj->getProperty(B_songImport::filePaths);
        std::vector<SongInfo> songs;
        juce::Array<juce::var> errorFiles;
        for (auto& filePath : filePaths) {
            auto path = juce::File(filePath.toString());
            auto result = dllManager::getInstance().getSongsManager().insertSong(path);
            if (result.has_value()) {
                songs.push_back(result.value());
            } else {
                errorFiles.add(path.getFileName());
            }
        }
        juce::DynamicObject::Ptr resultObj{new juce::DynamicObject()};
        resultObj->setProperty(B_songImport::songs, SongInfo::vector2VarArray(songs));
        resultObj->setProperty(B_songImport::errorFiles, errorFiles);
        return object2Uint8t(resultObj);
    }
}
