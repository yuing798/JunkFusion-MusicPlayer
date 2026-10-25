add_library(Models STATIC)

target_sources(Models
    PRIVATE 
        SongInfo.cpp
        PlayInfo.cpp
        AlbumInfo.cpp
        playlistInfo.cpp
)


target_include_directories(Models
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${FFMPEG_INCLUDE_DIRS}
)
target_link_directories(Models
    PRIVATE
        ${FFMPEG_LIBRARY_DIRS}
)