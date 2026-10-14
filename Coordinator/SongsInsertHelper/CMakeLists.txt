#歌曲插入的时候要进行非常多的逻辑机判断。除了insertSong本体其余都放在这个文件夹里面
target_sources(JunkFusionCoordinator
    PRIVATE
        WaveFormAnaly.cpp
        OnlineGetMatedata.cpp
)