#pragma once

#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_data_structures/juce_data_structures.h"
#include <spdlog/spdlog.h>
#include <string>

static const juce::File SerialCacheDirId{UserDirId.getChildFile("SerialCache")};
//用来序列化的ID
static constexpr const char* TreeRootId{"TreeRoot"};//树根
static constexpr const char* UIRootId{"UIRoot"};//UI缓存的根
static constexpr const char* APVTSRootId{"APVTSRoot"};//apvts根

class Serial{
private:
    juce::ValueTree treeRoot{TreeRootId};
    juce::ValueTree uiRoot{UIRootId};
    juce::ValueTree apvtsRoot{APVTSRootId};
    juce::File serialFile;
    
public:
    static Serial& getInstance() {
        static Serial instance; //首次调用时创建，程序结束时自动析构
        return instance;
    }
    void init();
    Serial();

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
        }else {
            auto logger{spdlog::get(LogSchedulerID)};
            logger->debug("变量{}不是算数类型",key.toString().toStdString());
        }
    }
    void save2disk();
    void loadInDisk();

    Serial(const Serial&) = delete;
    Serial& operator=(const Serial&) = delete;
    Serial(Serial&&) = delete;
    Serial& operator=(Serial&&) = delete;
};