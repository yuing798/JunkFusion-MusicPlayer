
#include "serial.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include <spdlog/spdlog.h>
Serial::Serial(){

}
void Serial::init(){
    if(!SerialCacheDirId.exists()) SerialCacheDirId.createDirectory();
    serialFile = SerialCacheDirId.getChildFile("serial.bin");
    if(!serialFile.existsAsFile()) serialFile.create();

    treeRoot.addChild(uiRoot,-1,nullptr);
    treeRoot.addChild(apvtsRoot,-1,nullptr);

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