#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ================================================================
// 平台特定头文件
// ================================================================
#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
    #include <signal.h>
    #include <sys/wait.h>
    #include <unistd.h>

#endif

// ================================================================
// 1. 通用类型定义 & 回调
// ================================================================
typedef void (*BackendExitCallback)(int exitCode);
static BackendExitCallback g_onExitCallback = nullptr;

// ================================================================
// 2. 跨平台状态封装
// ================================================================
struct ProcessState {
    std::mutex mtx;
    bool running = false;
    int exitCode = 0;

#ifdef _WIN32
    HANDLE hProcess = NULL;
    HANDLE hJobObject = NULL;
    HANDLE hWaitObject = NULL;
#else
    pid_t pid = -1;
    std::thread waitThread;
    bool threadRunning = false;
#endif
};
static ProcessState g_state;

// ================================================================
// 3. 辅助函数：UTF-8 → 宽字符（仅 Windows 需要）
// ================================================================
#ifdef _WIN32
static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], count);
    return wstr;
}
#endif

// ================================================================
// 4. 平台相关：启动进程
// ================================================================
static bool PlatformStartProcess(const std::string& backendPath, const std::string& cacheDir) {
#ifdef _WIN32
    // ---------- Windows 实现 ----------
    std::wstring wPath = Utf8ToWide(backendPath);
    std::wstring wCache = Utf8ToWide(cacheDir);
    // L"\""渲染出来是长字符串形式的引号"        反斜杠是转义的意思
    std::wstring cmdLine = L"\"" + wPath + L"\"" + L" --cache-dir=\"" + wCache;
    //    + L"\"" +
    //    L" --port=" + std::to_wstring(oscPort);
    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(L'\0');

    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessW(
            NULL,
            cmdBuffer.data(),
            NULL,
            NULL,
            FALSE,
            CREATE_SUSPENDED | CREATE_NO_WINDOW,
            NULL,
            NULL,
            &si,
            &pi
        )) {
        return false;
    }

    // 创建 Job Object（仅一次）
    if (g_state.hJobObject == NULL) {
        g_state.hJobObject = CreateJobObjectW(NULL, NULL);
        if (g_state.hJobObject) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
            jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(
                g_state.hJobObject,
                JobObjectExtendedLimitInformation,
                &jeli,
                sizeof(jeli)
            );
        }
    }

    g_state.hProcess = pi.hProcess;
    if (g_state.hJobObject) {
        AssignProcessToJobObject(g_state.hJobObject, pi.hProcess);
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    // 注册异步等待（进程退出时回调）
    RegisterWaitForSingleObject(
        &g_state.hWaitObject,
        g_state.hProcess,
        [](PVOID, BOOLEAN) {
            DWORD code = 0;
            if (g_state.hProcess) {
                GetExitCodeProcess(g_state.hProcess, &code);
                CloseHandle(g_state.hProcess);
                g_state.hProcess = NULL;
            }
            if (g_state.hWaitObject) {
                UnregisterWait(g_state.hWaitObject);
                g_state.hWaitObject = NULL;
            }
            if (g_onExitCallback) {
                g_onExitCallback((int)code);
            }
            std::lock_guard<std::mutex> lock(g_state.mtx);
            g_state.running = false;
        },
        NULL,
        INFINITE,
        WT_EXECUTEONLYONCE
    );
    return true;

#else
    // ---------- POSIX (macOS/Linux) 实现 ----------
    pid_t pid = fork();
    if (pid == -1) return false; // fork 失败

    if (pid == 0) {
        // --- 子进程 ---
        // 创建新进程组，便于后续批量杀进程
        setpgid(0, 0);
        // 构造命令行参数
        std::string portStr = std::to_string(oscPort);
        std::string cacheArg = "--cache-dir=" + cacheDir;
        std::string portArg = "--port=" + portStr;
        const char* argv[] = {backendPath.c_str(), cacheArg.c_str(), portArg.c_str(), nullptr};
        // 执行后端程序（替换当前进程）
        execvp(backendPath.c_str(), (char* const*)argv);
        // 如果 execvp 失败，退出子进程
        _exit(127);
    }

    // --- 父进程 ---
    g_state.pid = pid;
    g_state.running = true;

    // 启动等待线程
    g_state.waitThread = std::thread([pid]() {
        int status = 0;
        pid_t ret = waitpid(pid, &status, 0);
        if (ret == pid) {
            int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            if (g_onExitCallback) {
                g_onExitCallback(exitCode);
            }
            std::lock_guard<std::mutex> lock(g_state.mtx);
            g_state.running = false;
            g_state.exitCode = exitCode;
        }
    });
    return true;
#endif
}

// ================================================================
// 5. 平台相关：停止进程
// ================================================================
static void PlatformStopProcess() {
    std::lock_guard<std::mutex> lock(g_state.mtx);
    if (!g_state.running) return;

#ifdef _WIN32
    if (g_state.hProcess) {
        TerminateProcess(g_state.hProcess, 0);
        CloseHandle(g_state.hProcess);
        g_state.hProcess = NULL;
    }
    if (g_state.hWaitObject) {
        UnregisterWait(g_state.hWaitObject);
        g_state.hWaitObject = NULL;
    }
    // Job Object 会在库卸载时关闭，届时自动杀子进程
#else
    if (g_state.pid > 0) {
        // 杀掉整个进程组（包括子进程可能产生的孙进程）
        kill(-g_state.pid, SIGTERM);
        // 等待线程 join（但不要让此函数阻塞太久，可先设置超时）
        if (g_state.waitThread.joinable()) {
            g_state.waitThread.join();
        }
        g_state.pid = -1;
    }
#endif
    g_state.running = false;
}

// ================================================================
// 6. 库卸载时的清理（确保子进程被回收）
// ================================================================
#ifdef _WIN32
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_DETACH) {
        // 关闭 Job Object 触发 KILL_ON_JOB_CLOSE
        if (g_state.hJobObject) {
            CloseHandle(g_state.hJobObject);
            g_state.hJobObject = NULL;
        }
        // 其他句柄在回调中已关闭
    }
    return TRUE;
}
#else
// 使用 GCC/Clang 的析构函数属性，在库卸载时执行
__attribute__((destructor)) static void LibraryCleanup() { PlatformStopProcess(); }
#endif