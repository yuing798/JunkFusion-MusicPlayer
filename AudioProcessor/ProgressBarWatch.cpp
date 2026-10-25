#include "./ProgressBarWatch.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/mathUtils.hpp"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"

ProgressBarWatch::ProgressBarWatch(AudioProcessWorker& worker) : mWorker(worker) {}

void ProgressBarWatch::timerCallback() {
    double currentSeconds = mCurProgressBarSample / mSampleRate;
    Yvar obj = Yvar::object();
    obj.set(AudioMacro::currentPTS, currentSeconds);
    mWorker.sender->sendMessage(obj.dump());
}
void ProgressBarWatch::prepareToPlay(float sr) { mSampleRate = sr; }

void ProgressBarWatch::processBlock(int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        if (!isStop) {
            mCurProgressBarSample++;
        }
    }
}

void ProgressBarWatch::setTargetSeconds(float targetSeconds) {
    mCurProgressBarSample = MathUtils::msToSample(targetSeconds * 1000, mSampleRate);
}
void ProgressBarWatch::beginProgressBar(float targetSeconds) {
    setTargetSeconds(targetSeconds);
    if (!isTimerRunning()) startTimerHz(30);
    isStop = false;
}
void ProgressBarWatch::pauseProgressBar() {
    if (isTimerRunning()) stopTimer();
    isStop = true;
}
float ProgressBarWatch::getCurSeconds() {
    return MathUtils::sampleToMs(mCurProgressBarSample, mSampleRate) * 0.001;
}

ProgressBarWatch::~ProgressBarWatch() { pauseProgressBar(); }