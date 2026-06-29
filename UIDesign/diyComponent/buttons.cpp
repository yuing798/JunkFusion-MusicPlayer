#include "./buttons.hpp"
#include "./otherComponent.hpp"
#include "juce_events/juce_events.h"

void PlayStopButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) 
{
    // 1. 获取按钮区域，并向内缩进一点，防止图形贴边
    auto bounds = getLocalBounds().toFloat();
    auto width = bounds.getWidth();
    g.setColour(ycolor.shallowGrey);
    if(shouldDrawButtonAsHighlighted) g.fillRoundedRectangle(bounds,width / 2.0f);
    g.setColour(ycolor.midGrey);
    if(shouldDrawButtonAsDown) g.fillRoundedRectangle(bounds,width / 2.0f);
    
    // 2. 设置线条颜色和粗细
    g.setColour (juce::Colours::black);
    const float lineThickness = 3.0f;
    auto reducedWidth = (1.0f - 0.707f) * width;
    auto lineBounds = bounds.reduced(reducedWidth).reduced(1.0f);

    // 3. 根据按钮的开关状态（ToggleState）绘制不同的形状
    if (getToggleState())
    {
        // 状态为 true (开启/运行中)：绘制【停止方块】
        g.drawRect (lineBounds, lineThickness);
    }
    else
    {
        // 状态为 false (关闭/停止中)：绘制【播放三角】
        juce::Path triangle;
        triangle.startNewSubPath (lineBounds.getX(), lineBounds.getY());             // 左上角
        triangle.lineTo (lineBounds.getRight(), lineBounds.getCentreY());            // 右侧中心点
        triangle.lineTo (lineBounds.getX(), lineBounds.getBottom());                 // 左下角
        triangle.closeSubPath();                                             // 闭合路径

        g.strokePath (triangle, juce::PathStrokeType (lineThickness));
    }
}
yTextButton::yTextButton(){
    setClickingTogglesState(false);
    setToggleState(false,juce::dontSendNotification);
}
yTextButton::yTextButton(juce::String initText)
:yTextButton(){
    setButtonText(initText);
}

void yTextButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){
    auto local = getLocalBounds().toFloat();
    g.setColour(ycolor.shallowGrey);
    g.fillRoundedRectangle(local,6.0f);
    if(shouldDrawButtonAsHighlighted){
        g.setColour(ycolor.midGrey);
        g.fillRoundedRectangle(local,6.0f);
    }
    if(shouldDrawButtonAsDown){
        g.setColour(ycolor.darkGrey);
        g.fillRoundedRectangle(local,6.0f);
    }
    g.setColour (ycolor.black);
    g.setFont (juce::FontOptions { 18.0f });

    g.drawText (getButtonText(), local, juce::Justification::centred, true);
}
void plusButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){

    g.setColour(ycolor.midGrey);
    if(shouldDrawButtonAsHighlighted) g.fillAll();
    g.setColour(ycolor.darkGrey);
    if(shouldDrawButtonAsDown) g.fillAll();

    auto local = getLocalBounds().reduced(getWidth() / 4.0f);
    auto width = local.getWidth();
    auto height = local.getHeight();

    float plusShapeWidth = width / 4.0f;//加号可以视为两个纯黑色的矩形拼接起来的

    juce::Rectangle<float> rect1 = juce::Rectangle<float>(
        local.getX(),
        local.getY() + height * 0.5f - plusShapeWidth * 0.5f,
        width,
        plusShapeWidth);//上文说的矩形

    juce::Rectangle<float> rect2 = juce::Rectangle<float>(
        local.getX() + width * 0.5f - plusShapeWidth * 0.5f,
        local.getY(),
        plusShapeWidth,
        height
    );
    g.setColour(ycolor.black);
    g.fillRect(rect1);
    g.fillRect(rect2);


}

void minusButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){

    g.setColour(ycolor.midGrey);
    if(shouldDrawButtonAsHighlighted) g.fillAll();
    g.setColour(ycolor.darkGrey);
    if(shouldDrawButtonAsDown) g.fillAll();

    auto local = getLocalBounds().reduced(getWidth() / 4.0f);
    auto width = local.getWidth();
    auto height = local.getHeight();

    float plusShapeWidth = width / 4.0f;//加号可以视为两个纯黑色的矩形拼接起来的

    juce::Rectangle<float> rect1 = juce::Rectangle<float>(
        local.getX(),
        local.getY() + height * 0.5f - plusShapeWidth * 0.5f,
        width,
        plusShapeWidth);//上文说的矩形

    // juce::Rectangle<float> rect2 = juce::Rectangle<float>(
    //     local.getX() + width * 0.5f - plusShapeWidth * 0.5f,
    //     local.getY(),
    //     plusShapeWidth,
    //     height
    // );
    g.setColour(ycolor.black);
    g.fillRect(rect1);
    // g.fillRect(rect2);



}//单纯就是把加号的那一竖给去除，所以直接把加号的逻辑注释一部分就可以了

void noneButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){}

svgButton::svgButton(const juce::String& buttonName,std::unique_ptr<juce::Drawable> svg)
:juce::DrawableButton (buttonName, juce::DrawableButton::ImageFitted),
svg(std::move(svg)){

    setTooltip(buttonName);
}

void svgButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){

    auto baseColor = ycolor.shallowGrey;
    if(shouldDrawButtonAsHighlighted) baseColor = ycolor.midGrey;
    if(shouldDrawButtonAsDown) baseColor = ycolor.darkGrey;
    g.setColour(baseColor); 
    auto bounds = getLocalBounds().toFloat();
    auto width{bounds.getWidth()};
    auto reducedWidth{width * (1.0f - 0.707f)};//因为图形是圆形的，所以需要把边缘削去一部分
    g.fillRoundedRectangle(bounds, width / 2.0f);


    // 绘制 SVG，居中适应按钮区域
    svg.get()->drawWithin(g, bounds.reduced(reducedWidth).reduced(1.0f),
                            juce::RectanglePlacement::centred,
                              1.0f);
}

doubleSvgButton::doubleSvgButton(const juce::String& name1,std::unique_ptr<juce::Drawable> svg1,
    const juce::String& name2,std::unique_ptr<juce::Drawable> svg2)
:juce::DrawableButton ("", juce::DrawableButton::ImageFitted),
name1(name1),name2(name2),svg1(std::move(svg1)),svg2(std::move(svg2)){

    setClickingTogglesState(true);
    // 初始工具提示
    setTooltip(name1);
    // 状态变化时更新工具提示
    onStateChange = [this] {
        setTooltip(getToggleState() ? this->name1 : this->name2);
        //因为lambda不会隐式捕获类变量且发生了名称遮蔽(变量和传参重名)，所以需要this->
    };

}
void doubleSvgButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){

    auto baseColor = ycolor.shallowGrey;
    if(shouldDrawButtonAsHighlighted) baseColor = ycolor.midGrey;
    if(shouldDrawButtonAsDown) baseColor = ycolor.darkGrey;
    g.setColour(baseColor); 
    auto bounds = getLocalBounds().toFloat();
    auto width{bounds.getWidth()};
    auto reducedWidth{width * (1.0f - 0.707f)};//因为图形是圆形的，所以需要把边缘削去一部分
    g.fillRoundedRectangle(bounds, width / 2.0f);

    // 2. 根据 toggle 状态选择 SVG
    auto* svgToDraw = getToggleState() ? svg1.get() : svg2.get();
    if (svgToDraw != nullptr)
    {
        // 绘制 SVG，居中适应按钮区域
        svgToDraw->drawWithin(g, bounds.reduced(reducedWidth).reduced(1.0f),
                              juce::RectanglePlacement::centred,
                              1.0f);
    }
}

playPauseButton::playPauseButton()
:doubleSvgButton(U("播放"),
    juce::Drawable::createFromImageData(BinaryData::play_svg, BinaryData::play_svgSize),
    U("暂停"),
    juce::Drawable::createFromImageData(BinaryData::pause_svg, BinaryData::pause_svgSize)
){

}
upDownButton::upDownButton()
:doubleSvgButton(
    U("升序"),
    juce::Drawable::createFromImageData(BinaryData::up_svg, BinaryData::up_svgSize),
    U("降序"),
    juce::Drawable::createFromImageData(BinaryData::down_svg, BinaryData::down_svgSize)
){
    
}
