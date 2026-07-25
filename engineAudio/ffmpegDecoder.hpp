
#include "./AudioRingBuffer.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <string>

class FFmpegDecoder : public juce::Thread {
private:
    int numChannels{2};
    double sampleRate{defaultSampleRate};
    std::string path; // 文件路径
    AudioRingBuffer& ringBuffer;

public:
    void prepareToPlay(int, double);
    void run() override;
    std::string getPathBySongId(int64_t songId);
    FFmpegDecoder(AudioRingBuffer&);
    ~FFmpegDecoder();
};