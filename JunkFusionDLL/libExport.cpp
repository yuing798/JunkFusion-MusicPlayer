#include "./libExport.h"
#include "./fileManage/dbManager.hpp"
#include "constants.h"
#include "dllBridgeName.hpp"
#include "dllUtils.hpp"
#include "fileManage/dbModel.hpp"
#include "juce_core/juce_core.h"
#include <vector>

extern "C" {
    void dbInit() { dbManager::getInstance(); }
    int getAllSongCount() {
        auto count{dbManager::getInstance().getSongsManager().getTotalSongCount()};
        if (count.has_value()) {
            return count.value();
        } else {
            return 0;
        }
    }
    int toggleMyLike(long long songId) {
        return dbManager::getInstance().getSongsManager().reverseMyLike(songId);
    }
    const char* getAllSongs() {
        std::vector<SongInfo> songs = dbManager::getInstance().getSongsManager().getAllSongs();
        auto obj{new juce::DynamicObject()};
        obj->setProperty(B_getAllSongs::songsList, juce::var(SongInfo::vector2VarArray(songs)));
        return object2Uint8t(obj);
    }
    void saveComment(long long songId, const char* commentText) {
        dbManager::getInstance().getSongsManager().saveComment(commentText, songId);
        return;
    }
    void freeString(char* str) {
        if (str) {
            free(str);
        }
    }
    const char* someImport(const char* jsonStr) {
        auto obj = charPtr2object(jsonStr);
        juce::Array<juce::var> filePaths = obj->getProperty(B_songImport::filePaths);
        std::vector<SongInfo> songs;
        juce::Array<juce::var> errorFiles;
        for (auto& filePath : filePaths) {
            auto path = juce::File(filePath.toString());
            auto result = dbManager::getInstance().getSongsManager().insertSong(path);
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
    const char* getCacheDir() { return LocalDirId.getFullPathName().toRawUTF8(); }
}
