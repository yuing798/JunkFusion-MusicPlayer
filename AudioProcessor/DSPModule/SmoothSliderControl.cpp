#include "./SmoothSliderControl.hpp"

SmoothSliderControl::SmoothSliderControl(OscReceiver& oscReceiver, AudioProcessWorker& worker)
    : oscReceiver(oscReceiver), worker(worker) {

    worker.receiver->onMasterVolumeChange = oscReceiver.onMasterVolumeChange = [this](float value) {
        masterVolume.setTargetValue(value);
    };
    worker.receiver->onPitchShifterChange =
        oscReceiver.onPitchShifterChange = [this](float v) { pitchShifter.setTargetValue(v); };
    worker.receiver->onSpeedShifterChange =
        oscReceiver.onSpeedShifterChange = [this](float v) { speedShifter.setTargetValue(v); };
}
void SmoothSliderControl::prepareToPlay(float s) {
    masterVolume.reset(s, 0.005);
    pitchShifter.reset(s, 0.005);
    speedShifter.reset(s, 0.005);
    // SongChangeCrossFadeMs.reset(s, 0.005);
    // progressChangeCrossFadeMs.reset(s, 0.005);
    masterVolume.setCurrentAndTargetValue(0.5f);
    pitchShifter.setCurrentAndTargetValue(0.0f);
    speedShifter.setCurrentAndTargetValue(1.0f);
    // SongChangeCrossFadeMs.setCurrentAndTargetValue(40.0f);
    // progressChangeCrossFadeMs.setCurrentAndTargetValue(600.0f);
    sampleRate = (float)s;
}
// void SmoothSliderControl::processBlock(int numSamples) {
//     masterVolume.skip(numSamples);
//     pitchShifter.skip(numSamples);
//     speedShifter.skip(numSamples);
//     SongChangeCrossFadeMs.skip(numSamples);
//     progressChangeCrossFadeMs.skip(numSamples);
// }