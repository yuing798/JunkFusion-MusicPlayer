#include "./ffmpegDecoder.hpp"

void FFmpegDecoder::prepareToPlay(int n, double s) {
    numChannels = n;
    sampleRate = s;
}