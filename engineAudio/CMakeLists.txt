target_include_directories(JunkFusionBackecd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusionBackend PRIVATE
    AudioRingBuffer.cpp
    DeviceManager.cpp 
    ffmpegDecoder.cpp
    GodProcessor.cpp
)