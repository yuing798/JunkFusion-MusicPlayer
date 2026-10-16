#pragma once

#include "../CatagoryViewManage/ArtistsManage.hpp"
#include "../CatagoryViewManage/FileDirManage.hpp"
#include "../CatagoryViewManage/PlayListManage.hpp"
#include "Coordinator/CatagoryViewManage/AlbumsManage.hpp"
#include "Coordinator/CatagoryViewManage/songsManage.hpp"
#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include <string>

struct DbContext {
    CenterDbManage dbManage;
    SongsManage songsManage;
    AlbumsManage albumsManage;
    PlayListManage playListManage;
    FileDirManage fileDirManage;
    ArtistsManage artistsManage;

    DbContext(std::string dbPath)
        : dbManage(dbPath), songsManage(dbManage), albumsManage(dbManage), playListManage(dbManage),
          fileDirManage(dbManage), artistsManage(dbManage) {}
};