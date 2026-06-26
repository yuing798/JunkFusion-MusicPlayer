
//这些内容用来复制粘贴的，因为内容重复,到时候ctrlF替换一下名称就可以了

class StreamCardComponent : public juce::Component{
private:

public:

    StreamCardComponent();
    ~StreamCardComponent();
    void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StreamCardComponent)
};

StreamCardComponent::StreamCardComponent(){

}
StreamCardComponent::~StreamCardComponent(){

}
void StreamCardComponent::resized(){

}
void StreamCardComponent::paint(juce::Graphics& g){
    
}