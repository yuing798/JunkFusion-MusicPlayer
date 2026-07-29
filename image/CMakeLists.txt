juce_add_binary_data(
    JunkFusionBinaryData
    SOURCES
        JunkFusion.png
)
target_sources(JunkFusionBackend PRIVATE
    ImageManager.cpp
)