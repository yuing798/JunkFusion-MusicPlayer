#include "BinaryData.h"
#include "constants.h"
#include "juce_graphics/juce_graphics.h"
class YImageCache {
    
public:

    juce::Image originalIcon;      // 原始尺寸
    juce::Image icon50;            // 缩放后的 50x50

    YImageCache(){
        loadFromBinaryData();
    }
    void loadFromBinaryData() {
        originalIcon = juce::ImageCache::getFromMemory(BinaryData::JunkFusion_png, 
                                                       BinaryData::JunkFusion_pngSize);
        icon50 = originalIcon.rescaled(50,50);
    }
    static YImageCache& getInstance(){
        static YImageCache instance;
        return instance;
    }
    DONT_COPY_AND_MOVE(YImageCache)
};