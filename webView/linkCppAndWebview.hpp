
#include "juce_gui_extra/juce_gui_extra.h"
#include <memory>
class webView{
private:
    
public:
    std::unique_ptr<juce::WebBrowserComponent> web;
    webView();
    ~webView();
    
};