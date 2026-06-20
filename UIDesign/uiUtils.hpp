#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

//将中文的comboBoxId注册到comboBox中
void addChineseComboItem(juce::ComboBox&,std::vector<const char*>);