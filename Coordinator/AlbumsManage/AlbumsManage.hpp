#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
class AlbumsManage {
private:
    SQLite::Database* db;

public:
    AlbumsManage(SQLite::Database* db);
    const char* getAllAlbums();
    bool checkAlbum(juce::String title, juce::StringArray artists, juce::String hash);
    bool insertAlbum(juce::String title, juce::StringArray artists, juce::String hash);
};