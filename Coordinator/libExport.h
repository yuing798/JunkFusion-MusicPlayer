#ifndef LIB_EXPORT_H
#define LIB_EXPORT_H
#ifdef _WIN32
    #define __declspec(dllexport)
#else
    #define __attribute__((visibility("default")))
#endif

typedef void (*StringFunc)(const char* str);
typedef void (*DoubleFunc)(double);
typedef void (*VoidFunc)();

#ifdef __cplusplus
extern "C" {
#endif

    void dllInit(const char* cacheDirId,
                 const char* exeDirPtr); // dll初始化
    int toggleMyLike(long long songId);
    const char* getAllSongs();
    void saveComment(long long songId, const char* commentText);
    void freeString(char* str);
    const char* someImport(const char*);
    void closeBackend(); // dll注销
    void registerErrorSendCallback(StringFunc cb);
    void play(long long songId, double targetPTS);
    void pausePlay(); // 暂停播放
    void registerCurrentPTSCallback(DoubleFunc doubleFunc);
    void registerTimeDomainSpecInsertOver(StringFunc cb);
    const char* getTimeDomainSpecBySongId(long long songId);
    void sendSliderValue(const char* identify, double value, int isOSC);
    void registerOnPlayNextSong(VoidFunc cb);

#ifdef __cplusplus
}
#endif

#endif // LLB_EXPORT_H