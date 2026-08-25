#ifndef LLB_EXPORT_H
#define LLB_EXPORT_H
#ifdef _WIN32
    #define LIB_EXPORT __declspec(dllexport)
#else
    #define LIB_EXPORT __attribute__((visibility("default")))
#endif

typedef void (*StringFunc)(const char* str);
typedef void (*DoubleFunc)(double);

#ifdef __cplusplus
extern "C" {
#endif

    LIB_EXPORT void dllInit(const char* cacheDirId, const char* exeDirPtr); // dll初始化
    LIB_EXPORT int toggleMyLike(long long songId);
    LIB_EXPORT const char* getAllSongs();
    LIB_EXPORT void saveComment(long long songId, const char* commentText);
    LIB_EXPORT void freeString(char* str);
    LIB_EXPORT const char* someImport(const char*);
    LIB_EXPORT void closeBackend(); // dll注销
    LIB_EXPORT void registerErrorSendCallback(StringFunc cb);
    LIB_EXPORT void playNewSong(long long songId);
    LIB_EXPORT void continuePlay(); // 继续播放
    LIB_EXPORT void pausePlay();    // 暂停播放
    LIB_EXPORT void registerCurrentPTSCallback(DoubleFunc doubleFunc);
    LIB_EXPORT void seekTargetPTS(double targetSeconds); // 进度条移动到目标进度
    LIB_EXPORT void registerTimeDomainSpecInsertOver(StringFunc cb);
    LIB_EXPORT const char* getTimeDomainSpecBySongId(long long songId);
    LIB_EXPORT void setFirstPlay(long long songId, double currentPTS);

#ifdef __cplusplus
}
#endif

#endif // LLB_EXPORT_H