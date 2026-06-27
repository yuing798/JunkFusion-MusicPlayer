
#include "serial.hpp"
#include "constants.h"
Serial::Serial(){

}
void Serial::init(){
    if(!SerialCacheDirId.exists()) SerialCacheDirId.createDirectory();
    serialFile = SerialCacheDirId.getChildFile("serial.bin");
    if(!serialFile.existsAsFile()) serialFile.create();

    treeRoot.addChild(uiRoot,-1,nullptr);
    treeRoot.addChild(apvtsRoot,-1,nullptr);

}