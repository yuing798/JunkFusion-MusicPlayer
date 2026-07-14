
#include "constants.h"
#include "juce_core/juce_core.h"
#include "songsManage.hpp"
#include <SQLiteCpp/Database.h>
#include <memory>
class dbManager {
private:
    std::unique_ptr<SQLite::Database> db;
    std::unique_ptr<SongsManage> songs;

public:
    explicit dbManager() {
        juce::File dbFile{LocalDirId.getChildFile("JunkFusion.db")};
        db = std::make_unique<SQLite::Database>(
            dbFile.getFullPathName().toStdString(), 
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        
        //把 db 指针或引用传给各表
        songs = std::make_unique<SongsManage>(*db);
    }

    static dbManager& getInstance()
    {
        static dbManager instance;
        return instance;
    }
    DONT_COPY_AND_MOVE(dbManager)

    SongsManage& getSongsManager() { return *songs; }
};