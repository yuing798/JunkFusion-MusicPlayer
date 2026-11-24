#pragma once

#include "../CatagoryViewManage/ArtistsManage.hpp"
#include "../CatagoryViewManage/LyricsManage.hpp"
#include "../CatagoryViewManage/PlayListManage.hpp"
#include "../CatagoryViewManage/PluginChainPresetManage.hpp"
#include "../CatagoryViewManage/PluginPolicyManage.hpp"
#include "../CatagoryViewManage/SongFolderManage.hpp"
#include "Coordinator/CatagoryViewManage/AlbumsManage.hpp"
#include "Coordinator/CatagoryViewManage/songsManage.hpp"
#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include <string>

struct DbContext {
    CenterDbManage dbManage;
    SongsManage songsManage;
    AlbumsManage albumsManage;
    PlayListManage playListManage;
    SongFolderManage songFolderManage;
    ArtistsManage artistsManage;
    LyricsManage lyricsManage;

    // 插件：全局效果器链的预设 + 黑白名单
    PluginChainPresetManage pluginChainPresetManage;
    PluginPolicyManage pluginPolicyManage;

    DbContext(std::string dbPath)
        : dbManage(dbPath), songsManage(dbManage), albumsManage(dbManage), playListManage(dbManage),
          songFolderManage(dbManage), artistsManage(dbManage), lyricsManage(dbManage),
          pluginChainPresetManage(dbManage), pluginPolicyManage(dbManage) {}
};