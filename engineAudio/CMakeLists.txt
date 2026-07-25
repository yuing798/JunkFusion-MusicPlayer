target_include_directories(JunkFusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusion PRIVATE
    AudioRingBuffer.cpp
    DeviceManager.cpp 
)