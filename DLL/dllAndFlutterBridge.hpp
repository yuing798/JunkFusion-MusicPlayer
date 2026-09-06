//warning:this file will be generated auto,dont modify it by yourself

#pragma once
struct B_songImport{
    static constexpr const char* name = "songImport" ;
    static constexpr const char* filePaths = "filePaths" ;
    static constexpr const char* songs = "songs" ;
    static constexpr const char* errorFiles = "errorFiles" ;
    static constexpr const char* errorFileName = "errorFileName" ;
    static constexpr const char* errorFileReason = "errorFileReason" ;
};

struct B_toggleMyLike{
    static constexpr const char* name = "toggleMyLike" ;
    static constexpr const char* songId = "songId" ;
    static constexpr const char* successOrError = "successOrError" ;
};

struct B_getAllSongs{
    static constexpr const char* name = "getAllSongs" ;
    static constexpr const char* songsList = "songsList" ;
};

struct B_saveComment{
    static constexpr const char* name = "saveComment" ;
    static constexpr const char* text = "text" ;
    static constexpr const char* songId = "songId" ;
};

struct B_songInfo{
    static constexpr const char* name = "songInfo" ;
    static constexpr const char* songId = "songId" ;
    static constexpr const char* duration = "duration" ;
    static constexpr const char* title = "title" ;
    static constexpr const char* artist = "artist" ;
    static constexpr const char* album = "album" ;
    static constexpr const char* albumArtist = "albumArtist" ;
    static constexpr const char* genre = "genre" ;
    static constexpr const char* trackNumber = "trackNumber" ;
    static constexpr const char* discNumber = "discNumber" ;
    static constexpr const char* year = "year" ;
    static constexpr const char* composer = "composer" ;
    static constexpr const char* bitRate = "bitRate" ;
    static constexpr const char* sampleRate = "sampleRate" ;
    static constexpr const char* channelLayout = "channelLayout" ;
    static constexpr const char* bitDepth = "bitDepth" ;
    static constexpr const char* codecName = "codecName" ;
    static constexpr const char* aiGenre = "aiGenre" ;
    static constexpr const char* bpm = "bpm" ;
    static constexpr const char* key = "key" ;
    static constexpr const char* isMyLike = "isMyLike" ;
    static constexpr const char* comment = "comment" ;
    static constexpr const char* playNum = "playNum" ;
    static constexpr const char* hash = "hash" ;
};

struct B_getTimeDomainSpec{
    static constexpr const char* name = "getTimeDomainSpec" ;
    static constexpr const char* specList = "specList" ;
};

struct B_sliderParam{
    static constexpr const char* name = "sliderParam" ;
    static constexpr const char* masterVolume = "/master/volume" ;
};
