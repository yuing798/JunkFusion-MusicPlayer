#这一个文件夹做歌曲的分类视图管理，包括专辑分类，歌单分类，文件夹分类
target_sources(JunkFusionCoordinator
    PRIVATE
        AlbumsManage.cpp
        ArtistsManage.cpp
        SongFolderManage.cpp
        PlayListManage.cpp
        SongsManage.cpp
        LyricsManage.cpp
        # 插件：全局效果器链的预设 + 黑白名单
        PluginChainPresetManage.cpp
        PluginPolicyManage.cpp
)