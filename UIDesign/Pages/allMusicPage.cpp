#include "allMusicPage.hpp"

allMusicPage::allMusicPage(){
    if(!refreshSvg){
        addAndMakeVisible(refreshSvg.get());
    }
    addAndMakeVisible(selectFileButton);
    addAndMakeVisible(allMusicLabel);
    addAndMakeVisible(mViewPort);
    
}