#include "uiUtils.hpp"
#include "FontAbout/font.h"
#include <cstddef>

void addChineseComboItem(juce::ComboBox& combo,std::vector<const char*> vectors){
    for(size_t i = 0; i < vectors.size(); i++){
        combo.addItem(U(vectors[i]), i+1);
    }
}