#include "./DeviceManager.hpp"

void DeviceManager::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &manager) {
        // 这个if在底层声卡参数“改变完成并生效后”才会调用的，而不是在调节过程中
        if (auto* currentDevice = manager.getCurrentAudioDevice()) {
            double newSampleRate = currentDevice->getCurrentSampleRate();
            int newBufferSize = currentDevice->getCurrentBufferSizeSamples();

            auto newBitDepth = currentDevice->getCurrentBitDepth();
        }
    }
}

void DeviceManager::connectProcessor(GodProcessor* p) {
    player.setProcessor(p);
    manager.addAudioCallback(&player);
}
void DeviceManager::disconnectProcessor() {
    manager.removeAudioCallback(&player);
    player.setProcessor(nullptr);
}