
#include "constants.h"
class FFmpegDecoder {
private:
    void prepareToPlay(int, double);

public:
    int numChannels{2};
    double sampleRate{defaultSampleRate};
};