/**
 * @brief 跨平台命名共享内存（Windows / macOS / Linux）
 *
 * ── 用途 ──────────────────────────────────────────────────────────────
 * 音频进程和 Coordinator 之间传频谱数据：数据面直连、零拷贝，
 * 不经过 ZMQ（省一次序列化和线程切换）。
 *
 * ── 平台差异全部收在这里 ──────────────────────────────────────────────
 * | 平台            | 创建                        | 销毁                        |
 * |-----------------|-----------------------------|-----------------------------|
 * | Windows         | CreateFileMappingW          | UnmapViewOfFile/CloseHandle |
 * | macOS / Linux   | shm_open + ftruncate + mmap | munmap + shm_unlink         |
 *
 * 上层（音频进程 / Coordinator）只用 create/open/close，不写任何 #ifdef。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     // 创建方（Coordinator）
 *     SharedMemory shm;
 *     shm.create("JunkFusion.Spectrum.1234", bytes);
 *     auto name = shm.fullName();        // 发给音频进程的就是这个完整名字
 *
 *     // 使用方（音频进程）
 *     SharedMemory shm;
 *     shm.open(nameFromCoordinator, bytes);
 *
 * ⚠️ Windows 下名字带 "Local\" 前缀（只在当前登录会话可见）；
 *    POSIX 下必须以 '/' 开头，所以 create() 会自动加前缀，
 *    fullName() 返回的是**加过前缀的最终名字**，open() 直接用它。
 *
 * ⚠️ POSIX 的共享内存对象不会随进程退出自动消失，析构时会 shm_unlink；
 *    但持有方异常崩溃时可能残留，所以数据头里还有 magic/version 做校验。
 */
#pragma once

#include <cstddef>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

class SharedMemory {
public:
    SharedMemory() = default;
    ~SharedMemory() { close(); }

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    /**
     * @brief 创建（已存在就复用）一块命名共享内存
     *
     * @param baseName 不带平台前缀的名字，比如 "JunkFusion.Spectrum.1234"
     * @param bytes 字节数
     * @return 成功与否；成功时 fullName() 是最终名字
     */
    bool create(const std::string& baseName, size_t bytes) {
        close();
        mSize = bytes;

#ifdef _WIN32
        mName = "Local\\" + baseName;
        mMapFile = ::CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            static_cast<DWORD>(bytes >> 32),
            static_cast<DWORD>(bytes & 0xFFFFFFFFULL),
            mName.c_str()
        );
        if (mMapFile == nullptr) {
            mName.clear();
            return false;
        }
        mData = ::MapViewOfFile(mMapFile, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
        if (mData == nullptr) {
            ::CloseHandle(mMapFile);
            mMapFile = nullptr;
            mName.clear();
            return false;
        }
        return true;
#else
        mName = "/" + baseName;
        for (char& ch : mName) {
            if (ch == '\\') ch = '.';
        }

        const int fd = ::shm_open(mName.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd < 0) {
            mName.clear();
            return false;
        }
        if (::ftruncate(fd, static_cast<off_t>(bytes)) != 0) {
            ::close(fd);
            ::shm_unlink(mName.c_str());
            mName.clear();
            return false;
        }
        mData = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ::close(fd);
        if (mData == MAP_FAILED) {
            mData = nullptr;
            ::shm_unlink(mName.c_str());
            mName.clear();
            return false;
        }
        mOwner = true;
        return true;
#endif
    }

    /**
     * @brief 打开一块已经存在的共享内存
     *
     * @param fullName create() 之后 fullName() 返回的名字（带平台前缀）
     * @param bytes 字节数，必须和创建方一致
     */
    bool open(const std::string& fullName, size_t bytes) {
        close();
        mName = fullName;
        mSize = bytes;

#ifdef _WIN32
        mMapFile = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mName.c_str());
        if (mMapFile == nullptr) {
            mName.clear();
            return false;
        }
        mData = ::MapViewOfFile(mMapFile, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
        if (mData == nullptr) {
            ::CloseHandle(mMapFile);
            mMapFile = nullptr;
            mName.clear();
            return false;
        }
        return true;
#else
        const int fd = ::shm_open(mName.c_str(), O_RDWR, 0666);
        if (fd < 0) {
            mName.clear();
            return false;
        }
        mData = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        ::close(fd);
        if (mData == MAP_FAILED) {
            mData = nullptr;
            mName.clear();
            return false;
        }
        mOwner = false; // 只负责打开，不负责 unlink
        return true;
#endif
    }

    void close() {
#ifdef _WIN32
        if (mData != nullptr) {
            ::UnmapViewOfFile(mData);
            mData = nullptr;
        }
        if (mMapFile != nullptr) {
            ::CloseHandle(mMapFile);
            mMapFile = nullptr;
        }
#else
        if (mData != nullptr) {
            ::munmap(mData, mSize);
            mData = nullptr;
        }
        if (mOwner && !mName.empty()) {
            ::shm_unlink(mName.c_str());
            mOwner = false;
        }
#endif
        mSize = 0;
        mName.clear();
    }

    void* data() const noexcept { return mData; }
    size_t size() const noexcept { return mSize; }
    bool isValid() const noexcept { return mData != nullptr; }
    const std::string& fullName() const noexcept { return mName; }

private:
    std::string mName;
    size_t mSize{0};
    void* mData{nullptr};

#ifdef _WIN32
    HANDLE mMapFile{nullptr};
#else
    bool mOwner{false};
#endif
};
