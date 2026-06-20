#include "MainComponent.h"
#include "UIDesign/UISet.h"

//==============================================================================
MainComponent::MainComponent()
:tooltipWindow(this,400){
    setLookAndFeel(&mLook);
    addAndMakeVisible(mLeftComponent);
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
    auto local = getLocalBounds();
    auto height = local.getHeight();
    auto width = local.getWidth();
    mLeftComponent.setBounds(local.removeFromLeft(180));
}
MainComponent::~MainComponent(){
    setLookAndFeel(nullptr);
}
