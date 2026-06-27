
#include "serial.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_data_structures/juce_data_structures.h"
#include <spdlog/spdlog.h>
Serial::Serial(){

}
void Serial::init(){
    if(!SerialCacheDirId.exists()) SerialCacheDirId.createDirectory();
    serialFile = SerialCacheDirId.getChildFile("serial.bin");
    if(!serialFile.existsAsFile()){
        serialFile.create();

        treeRoot.addChild(uiRoot,-1,nullptr);
        treeRoot.addChild(apvtsRoot,-1,nullptr);
        auto logger{spdlog::get(LogSchedulerID)};
        if(logger) logger->info("初次打开文件，将创建序列化文件");
    }else{
        loadInDisk();
    }

}
void Serial::save2disk(){
    juce::TemporaryFile tempFile(serialFile);
    bool writeState{false};
    
    {
        juce::FileOutputStream stream(tempFile.getFile());
        if(stream.openedOk()){
            treeRoot.writeToStream(stream);
            stream.flush();
            writeState = true;
            
        }else{
            auto logger{spdlog::get(LogSchedulerID)};
            if(logger) logger->error("磁盘存储流打开失败");
        }
    }
    if(writeState){
        if(!tempFile.overwriteTargetFileWithTemporary()){
            auto logger{spdlog::get(LogSchedulerID)};
            if(logger) logger->error("序列化文件覆盖失败");
        }
    }
}
void Serial::loadInDisk(){
    juce::FileInputStream stream(serialFile);
    treeRoot = juce::ValueTree::readFromStream(stream);
    if(treeRoot.isValid()){
        uiRoot = treeRoot.getChildWithName(UIRootId);
        apvtsRoot = treeRoot.getChildWithName(APVTSRootId);
        auto logger{spdlog::get(LogSchedulerID)};
        if(logger) logger->info("反序列文件读取成功");
    }else{
        auto logger{spdlog::get(LogSchedulerID)};
        if(logger) logger->error("二进制数据解析失败");
    }

}