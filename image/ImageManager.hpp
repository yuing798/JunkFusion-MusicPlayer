#include "BinaryData.h"
#include "constants.h"
#include "juce_graphics/juce_graphics.h"
#include <cstddef>
#include <vector>
class ImageManager {
    
public:

    juce::Image originalIcon;      // 原始尺寸
    juce::Image icon50;            // 缩放后的 50x50

    ImageManager();
    void loadFromBinaryData();//将应用级图片注册到内存中
    std::vector<std::byte> png2ByteVector(juce::Image&);//将png加载为byteVector
    std::vector<std::byte> jpg2ByteVector(juce::Image&);//将jpg加载为byteVector
    juce::Image clipMode(juce::Image& img,int targetWidth = 50,int targetHeight = 50);//裁剪图片

    static ImageManager& getInstance(){
        static ImageManager instance;
        return instance;
    }
    DONT_COPY_AND_MOVE(ImageManager)
};