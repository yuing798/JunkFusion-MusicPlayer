#pragma once

#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_data_structures/juce_data_structures.h"
#include <atomic>
#include <spdlog/spdlog.h>
#include <string>

static const juce::File SerialCacheDirId{UserDirId.getChildFile("SerialCache")};
//用来序列化的ID
static constexpr const char* TreeRootId{"TreeRoot"};//树根
static constexpr const char* UIRootId{"UIRoot"};//UI缓存的根
static constexpr const char* APVTSRootId{"APVTSRoot"};//apvts根
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};//窗口宽度，窗口长度
// static const juce::Identifier& SERIALwindowHeight{"windowHeight"};
static const juce::Identifier& SERIAL_allMusicSeqWays{"allMusicSeqWays"};//所有歌曲页的排序方式
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};
// static const juce::Identifier& SERIALwindowWidth{"windowWidth"};

//---------------------------------------------------------------------------
class ScopedWriteGuard//数据写入守卫
{
public:
    ScopedWriteGuard(std::atomic<bool>& flag) : flagRef(flag) {}
    
    // 尝试上锁
    bool tryLock() 
    { 
        bool expected = false;
        return flagRef.compare_exchange_strong(expected, true); 
        /*
        读取 flagRef 的当前值。
        如果当前值 等于 expected（即等于 false），则将 flagRef 设置为 true，并返回 true。
        如果当前值 不等于 expected（即已经是 true），则将 expected 更新为当前值（此时变成 true），并返回 false。
        第二个参数:
        比较当前值是否等于 expected。
        如果相等：把当前值替换为 desired（即第二个参数），返回 true。
        如果不相等：把当前值写入 expected（让 expected 变成最新的值），返回 false。
        */
    }

    // 析构时自动释放
    ~ScopedWriteGuard() 
    { 
        flagRef.store(false); 
    }

    // 禁止拷贝
    ScopedWriteGuard(const ScopedWriteGuard&) = delete;
    ScopedWriteGuard& operator=(const ScopedWriteGuard&) = delete;

private:
    std::atomic<bool>& flagRef;
};
//-------------------------------------------------------------------------------------------------------

class Serial : public juce::Timer{
private:
    juce::ValueTree treeRoot{TreeRootId};
    juce::ValueTree uiRoot{UIRootId};
    juce::ValueTree apvtsRoot{APVTSRootId};
    juce::File serialFile;
    std::atomic<bool> isWriting;//是否正在写入文件，防止打断
    
public:
    static Serial& getInstance() {
        static Serial instance; //首次调用时创建，程序结束时自动析构
        return instance;
    }
    void init();
    Serial();
    ~Serial();

    template<typename Ty>
    void saveUISingleValue2RAM(juce::Identifier& key,Ty value){//保存单一UI变量
        if constexpr (std::is_arithmetic_v<Ty>){
            uiRoot.setProperty(key, value, nullptr);
        }
    }

    template<typename Ty>
    Ty loadUISingleValueInRAM(const juce::Identifier& key,Ty defaultValue){
        if constexpr (std::is_arithmetic_v<Ty>){
            return uiRoot.getProperty(key,defaultValue);
        }
    }
    void save2disk();
    void loadInDisk();
    void timerCallback() override;//自动保存使用的计时器

    Serial(const Serial&) = delete;
    Serial& operator=(const Serial&) = delete;
    Serial(Serial&&) = delete;
    Serial& operator=(Serial&&) = delete;
};