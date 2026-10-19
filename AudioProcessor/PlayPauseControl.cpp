#include "./PlayPauseControl.hpp"
#include "PlayPauseControl.hpp"
#include "juce_audio_basics/juce_audio_basics.h"

void PlayPauseControl::prepareToPlay(float sampleRate) {
    smoothPlayPause.reset(sampleRate, 0.5f);
    smoothPlayPause.setCurrentAndTargetValue(0.0f);
}
void PlayPauseControl::setPlaybackState(PlayPauseState playOrPause, BeginEndState beginOrEnd) {
    if (playOrPause == PlayPauseState::Play) {
        if (beginOrEnd == BeginEndState::Begin) {
            smoothPlayPause.setCurrentAndTargetValue(1.0f);
        } else {
            smoothPlayPause.setTargetValue(1.0f);
        }
        isFullMute = false;
        hadPlayStatle = false;
    } else {
        if (beginOrEnd == BeginEndState::End) {
            smoothPlayPause.setCurrentAndTargetValue(0.0f);
        } else {
            smoothPlayPause.setTargetValue(0.0f);
        }
        isFullMute = false;
        hadPlayStatle = false;
    }
}

void PlayPauseControl::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) {

    for (int i = 0; i < numSamples; i++) {
        auto curGain = smoothPlayPause.getNextValue();
        if (curGain == 0.0f && isFullMute == false) {
            isFullMute = true;
            if (onPauseOver) onPauseOver();
        }
        if (curGain == 1.0f && !hadPlayStatle) {
            hadPlayStatle = true;
            if (onPlayStable) onPlayStable();
        }
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto ptr = buffer.getWritePointer(ch);
            ptr[i] *= curGain;
        }
    }
}