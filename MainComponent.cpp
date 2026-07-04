#include "MainComponent.h"
#include "UIDesign/diyComponent/otherComponent.hpp"
#include "serial.hpp"

//==============================================================================
MainComponent::MainComponent(){

    setSize (1400, 700); 
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.setColour(ycolor.white);
    g.fillAll();
}

void MainComponent::resized()
{

}

MainComponent::~MainComponent(){
}
