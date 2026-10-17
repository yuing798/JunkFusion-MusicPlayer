#include "./CrossFadeArea.hpp"
#include "Utils/mathUtils.hpp"

void CrossFadeArea::prepareToPlay(float sr) { mSampleRate = sr; }

void CrossFadeArea::processBlock(
    std::array<juce::AudioBuffer<float>*, 2> inputs,
    juce::AudioBuffer<float>& output,
    int numSamples
) {
    for (int i = 0; i < numSamples; i++) {

        if (isCrossFade) {

            int crossFadeLengthMs{0};
            if (mIsSongChange) {
                crossFadeLengthMs = songChangeMs;
            } else {
                crossFadeLengthMs = seekMs;
            }
            int crossFadeLength = MathUtils::msToSample(crossFadeLengthMs, mSampleRate);
            float fadeProcess = (float)currentCrossFadeIndex / (float)crossFadeLength;
            auto sinGain = MathUtils::getLinearInterpolator(
                MathUtils::fadeInSinTable.data(),
                MathUtils::fadeInSinTable.size(),
                fadeProcess
            );
            auto cosGain = MathUtils::getLinearInterpolator(
                MathUtils::fadeOutCosTable.data(),
                MathUtils::fadeOutCosTable.size(),
                fadeProcess
            );
            for (int ch = 0; ch < output.getNumChannels(); ch++) {
                auto mainPtr = inputs[mainDuckIndex]->getReadPointer(ch);
                auto deputyPtr = inputs[!mainDuckIndex]->getReadPointer(ch);
                auto outPtr = output.getWritePointer(ch);
                outPtr[i] = mainPtr[i] * sinGain + deputyPtr[i] * cosGain;
            }
            if (fadeProcess >= 1.0f) {
                isCrossFade = false;
                currentCrossFadeIndex = 0;
                if (onCrossFadeEnd) onCrossFadeEnd();
            }
            currentCrossFadeIndex++;
        } else {
            for (int ch = 0; ch < output.getNumChannels(); ch++) {
                auto inPtr = inputs[mainDuckIndex]->getReadPointer(ch);
                auto outPtr = output.getWritePointer(ch);
                outPtr[i] = inPtr[i];
            }
        }
    }
}

void CrossFadeArea::setCrossFadeBegin(bool isSongChange) {
    mainDuckIndex = !mainDuckIndex;
    currentCrossFadeIndex = 0;
    isCrossFade = true;
    mIsSongChange = isSongChange;
}