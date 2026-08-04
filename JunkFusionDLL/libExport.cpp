#include "./libExport.h"
#include "./fileManage/dbManager.hpp"
#include "dllUtils.hpp"

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
}
