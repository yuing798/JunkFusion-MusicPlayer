#pragma once
#include <juce_gui_basics/juce_gui_basics.h>


void showMediaFileChooser(std::function<void(const juce::File&)> onFileSelected,juce::Component* parentComponent = nullptr);