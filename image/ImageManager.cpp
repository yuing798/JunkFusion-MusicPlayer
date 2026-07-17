#include "ImageManager.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include <spdlog/spdlog.h>

std::vector<std::byte> ImageManager::png2MemoryAndFile(juce::Image img,const juce::File& path){

    // 1. 创建 PNG 编码器
    juce::PNGImageFormat pngFormat;

    // 2. 创建内存输出流，用于接收编码后的数据
    juce::MemoryOutputStream memoryStream;

    // 3. 将图像编码为 PNG 并写入内存流
    if (pngFormat.writeImageToStream(img, memoryStream))
    {
        // 4. 获取编码后的数据
        const void* data = memoryStream.getData();
        size_t size = memoryStream.getDataSize();

        if(path.getFullPathName().isNotEmpty() && !path.existsAsFile()){
            juce::FileOutputStream fileStream(path);//如果文件本身不存在，juce::FileOutputStream 会自动创建该文件
            if(fileStream.openedOk()){
                fileStream.write(data,size);
                fileStream.flush();
            }else{
                auto logger = spdlog::get(LogUiID);
                logger->error("error to write image into file {}",path.getFullPathName().toStdString());
            }
        }

        // 5. 转换为 std::vector<std::byte>
        const std::byte* byteData = static_cast<const std::byte*>(data);
        std::vector<std::byte> vec(byteData, byteData + size);
        return vec;

    }
    else
    {
        auto logger = spdlog::get(LogUiID);
        logger->error("error to write img into stream");
        return {};
    }
}

juce::Image ImageManager::clipMode(juce::Image& img,int targetWidth,int targetHeight){
    // 防止空图片或目标尺寸为0
    const int srcW = img.getWidth();
    const int srcH = img.getHeight();
    if (srcW == 0 || srcH == 0 || targetWidth == 0 || targetHeight == 0) {
        return {};
    }

    //计算覆盖目标所需的最小缩放比例
    const float scaleX = static_cast<float>(targetWidth) / static_cast<float>(srcW);
    const float scaleY = static_cast<float>(targetHeight) / static_cast<float>(srcH);
    // 取两者中的较大值：保证缩放后图片的短边一定大于等于目标短边
    const float scale = juce::jmax(scaleX, scaleY);

    // 计算“放大后的临时尺寸”
    const int scaledW = static_cast<int>(std::ceil(srcW * scale));
    const int scaledH = static_cast<int>(std::ceil(srcH * scale));

    //执行高质量缩放
    juce::Image scaledImage = img.rescaled(scaledW, scaledH, juce::Graphics::highResamplingQuality);

    //计算居中的裁剪起始点
    const int cropX = (scaledW - targetWidth) / 2;
    const int cropY = (scaledH - targetHeight) / 2;
    juce::Rectangle<int> cropArea(cropX, cropY, targetWidth, targetHeight);

    //裁剪并返回最终结果
    return scaledImage.getClippedImage(cropArea);
}
std::vector<std::byte> ImageManager::jpg2MemoryAndFile(juce::Image& img, int Q,const juce::File& path){

    juce::JPEGImageFormat jpegFormat;

    // 设置 JPEG 质量
    jpegFormat.setQuality(Q); 

    juce::MemoryOutputStream memoryStream;

    if (jpegFormat.writeImageToStream(img, memoryStream))
    {
        
        const void* data = memoryStream.getData();
        size_t size = memoryStream.getDataSize();

        if(path.getFullPathName().isNotEmpty() && !path.existsAsFile()){
            juce::FileOutputStream fileStream(path);//如果文件本身不存在，juce::FileOutputStream 会自动创建该文件
            if(fileStream.openedOk()){
                fileStream.write(data,size);
                fileStream.flush();
            }else{
                auto logger = spdlog::get(LogUiID);
                logger->error("error to write image into file {}",path.getFullPathName().toStdString());
            }
        }

        const std::byte* byteData = static_cast<const std::byte*>(data);
        std::vector<std::byte> vec(byteData, byteData + size);
        return vec;
    }else{
        auto logger = spdlog::get(LogUiID);
        logger->error("error to write img into memory stream");
        return {};
    }
}