
//这些内容用来复制粘贴的，因为内容重复,到时候ctrlF替换一下名称就可以了

class LoadingGreyBlock : public juce::Component{
private:

public:

    LoadingGreyBlock();
    ~LoadingGreyBlock();
    void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoadingGreyBlock)
};

LoadingGreyBlock::LoadingGreyBlock(){

}
LoadingGreyBlock::~LoadingGreyBlock(){

}
void LoadingGreyBlock::resized(){

}
void LoadingGreyBlock::paint(juce::Graphics& g){
    
}