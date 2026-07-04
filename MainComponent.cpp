#include "MainComponent.h"
#include "UIDesign/diyComponent/otherComponent.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "serial.hpp"
#include <memory>

//这里定义所有的桥接函数ID
static constexpr const char* fileInputId{"fileInput"};//音频文件导入应用
static constexpr const char* dirScanId{"dirScan"};//文件夹扫描

//==============================================================================
MainComponent::MainComponent(){
    juce::WebBrowserComponent::Options options;
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled(true)
        .withNativeFunction(
            fileInputId,
            [this](
                const juce::Array<juce::var>& args,
                juce::WebBrowserComponent::NativeFunctionCompletion complete
            ){

            }
        );
    
    web = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*web);
    #ifdef JUCE_DEBUG
    web->goToURL("http://127.0.0.1:5173");
    #else
        //这里到时候放置release版本的goToURL，因为http://127.0.0.1:5173是开发者专用的
    #endif
}

void MainComponent::resized()
{
    web->setBounds(getLocalBounds());
}

MainComponent::~MainComponent(){
}
