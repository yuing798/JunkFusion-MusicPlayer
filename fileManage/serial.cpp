
// #include "serial.hpp"
// #include "constants.h"
// #include "juce_core/juce_core.h"
// #include "juce_data_structures/juce_data_structures.h"
// #include <spdlog/spdlog.h>
// Serial::Serial(){

// }
// void Serial::init(){
//     serialFile = LocalDirId.getChildFile("serial.bin");
//     if(!serialFile.existsAsFile()){
//         serialFile.create();

//         treeRoot.addChild(uiRoot,-1,nullptr);
//         treeRoot.addChild(apvtsRoot,-1,nullptr);
//         auto logger{spdlog::get(LogSchedulerID)};
//         if(logger) logger->info("初次打开文件，将创建序列化文件");
//     }else{
//         loadInDisk();
//     }
//     startTimer(60000);//一分钟自动保存一次

// }
// void Serial::save2disk(){

//     ScopedWriteGuard guard(isWriting);
//     if (!guard.tryLock())
//         return; // 正在写入，直接返回

//     juce::TemporaryFile tempFile(serialFile);
    
//     {
//         juce::FileOutputStream stream(tempFile.getFile());
//         if(stream.openedOk()){
//             treeRoot.writeToStream(stream);
//             stream.flush();
//             isWriting.store(true);
            
//         }else{
//             auto logger{spdlog::get(LogSchedulerID)};
//             if(logger) logger->error("磁盘存储流打开失败");
//         }
//     }
//     if(isWriting.load()){
//         if(!tempFile.overwriteTargetFileWithTemporary()){
//             auto logger{spdlog::get(LogSchedulerID)};
//             if(logger) logger->error("序列化文件覆盖失败");
//         }
//     }
// }
// void Serial::loadInDisk(){
//     juce::FileInputStream stream(serialFile);
//     treeRoot = juce::ValueTree::readFromStream(stream);
//     if(treeRoot.isValid()){
//         uiRoot = treeRoot.getChildWithName(UIRootId);
//         apvtsRoot = treeRoot.getChildWithName(APVTSRootId);
//         auto logger{spdlog::get(LogSchedulerID)};
//         if(logger) logger->info("反序列文件读取成功");
//     }else{
//         auto logger{spdlog::get(LogSchedulerID)};
//         if(logger) logger->error("二进制数据解析失败");
//     }
// }
// void Serial::timerCallback(){
//     save2disk();
// }
// Serial::~Serial(){
    
// }