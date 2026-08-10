#ifndef LLB_EXPORT_H
#define LLB_EXPORT_H

#ifdef _WIN32
    #define LIB_EXPORT __declspec(dllexport)
#else
    #define LIB_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

    LIB_EXPORT void dllInit(const char* cacheDirId);
    LIB_EXPORT int getAllSongCount();
    LIB_EXPORT int toggleMyLike(long long songId);
    LIB_EXPORT const char* getAllSongs();
    LIB_EXPORT void saveComment(long long songId, const char* commentText);
    LIB_EXPORT void freeString(char* str);
    LIB_EXPORT const char* someImport(const char*);

#ifdef __cplusplus
}
#endif

#endif // LLB_EXPORT_H