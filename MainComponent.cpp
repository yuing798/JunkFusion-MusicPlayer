#include "MainComponent.h"
#include "UIDesign/UISet.h"

//==============================================================================
MainComponent::MainComponent()
{
    setLookAndFeel(&mLook);
    addAndMakeVisible(mLeftSelectedComponent);
    setSize (600, 400);
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
    mLeftSelectedComponent.setBounds(local.removeFromLeft(180));
}
MainComponent::~MainComponent(){
    setLookAndFeel(nullptr);
}
