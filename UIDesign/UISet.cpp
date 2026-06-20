#include "UISet.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <utility>

YColor ycolor;

YLookAndFeel::YLookAndFeel(){
    setColour(juce::PopupMenu::backgroundColourId,ycolor.midGrey);
    setColour(juce::PopupMenu::textColourId, ycolor.black);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ycolor.darkGrey);
    setColour(juce::PopupMenu::highlightedTextColourId, ycolor.black);

    // customTypeface = juce::Typeface::findSystemTypeface();
}

juce::Typeface::Ptr YLookAndFeel::getTypefaceForFont (const juce::Font& font){
    // 用 static 确保整个生命周期内只调用一次 findSystemTypeface()
    static auto systemUIFace = juce::Typeface::findSystemTypeface();

    if (systemUIFace != nullptr)
        return systemUIFace;

    // 如果获取失败（极罕见），回退到 JUCE 默认字体
    return LookAndFeel_V4::getTypefaceForFont (font);
}

void YLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height){
    auto bounds = juce::Rectangle<float> (0, 0, width,  height);
    g.setColour (ycolor.midGrey);
    g.fillRoundedRectangle (bounds, 5.0f); // 圆角背景

    g.setColour(ycolor.greyBlue);
    g.drawRoundedRectangle(
        bounds.getX(),
        bounds.getY(),
        bounds.getWidth(),
        bounds.getHeight(),
        5.0f,
        3.0f
    );

    g.setColour (ycolor.black);
    g.setFont (15.0f);
    g.drawFittedText (
        text, 
        bounds.reduced(5.0f).toNearestInt(), 
        juce::Justification::centred, 
        5
    );
}

juce::Font YLookAndFeel::getComboBoxFont(juce::ComboBox& box){
    return juce::Font (juce::FontOptions().withHeight (16.0f));
}

juce::Font YLookAndFeel::getPopupMenuFont(){
    return juce::Font (juce::FontOptions().withHeight (16.0f));
}

void YLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                    juce::Slider& slider)
{
    // 1. 计算基本几何参数
    auto radius = (juce::jmin (width, height) / 2.0f) - 10.0f; // 留出一点边缘防止切边
    auto centreX = x + width * 0.5f;
    auto centreY = y + height * 0.5f;

    // 当前指针所处的绝对角度
    auto currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    // 轨道的粗细
    auto trackThickness = 6.0f; 

    // =================================================================
    // 部分一：绘制【未滑过】的背景轨道（从当前角度到终点角度）
    // =================================================================
    juce::Path backgroundTrack;
    backgroundTrack.addCentredArc (
        centreX, 
        centreY, 
        radius, 
        radius,                                     
        0.0f,//起始时的绝对旋转角度
        currentAngle, 
        rotaryEndAngle, 
        true//是否作为一条全新路径的起点
    );

    g.setColour (ycolor.greyGreen); // 灰绿色（未滑过）
    g.strokePath (//给路径描边
        backgroundTrack, 
        juce::PathStrokeType (//描边的样式（比如线条有多粗、线头是方还是圆等）
            trackThickness,                                                             
            juce::PathStrokeType::curved,                                       
            juce::PathStrokeType::rounded
        )
    );

    // =================================================================
    // 部分二：绘制【已滑过】的激活轨道（从起点角度到当前角度）
    // =================================================================
    juce::Path filledTrack;
    filledTrack.addCentredArc (
        centreX, 
        centreY, 
        radius, 
        radius, 

        0.0f, 
        rotaryStartAngle, 
        currentAngle, 
        true
    );

    g.setColour (ycolor.greyBlue);
    g.strokePath (
        filledTrack, 
        juce::PathStrokeType (
            trackThickness,                                                         
            juce::PathStrokeType::curved,                                                         
            juce::PathStrokeType::rounded
        )
    );

    // =================================================================
    // 部分三：绘制【自定义形状】的指针
    // =================================================================
    // 我们以原点 (0,0) 为假想的圆心来设计指针形状，随后通过矩阵旋转到正确的位置
    juce::Path pointer;

    // 示例：画一个精美的三角形指针（顶点指向外圈轨道）
    float pointerWidth = 8.0f;
    float pointerLength = 14.0f;

    // 在垂直向上（12点钟方向）画一个菱形
    pointer.startNewSubPath(centreX,centreY);
    pointer.lineTo(centreX - pointerWidth * 0.5f,centreY - pointerLength * 0.5f);
    pointer.lineTo(centreX,centreY - pointerLength);
    pointer.lineTo(centreX + pointerWidth * 0.5f,centreY - pointerLength * 0.5f);
    pointer.closeSubPath();

    //将指针旋转到当前角度，并平移到旋钮中心
    pointer.applyTransform (//仿射变换
        juce::AffineTransform::rotation (currentAngle).translated (centreX, centreY)
    );//先旋转到特定角度，然后平移到中心点,原始图形的方向必须指向12点钟方向

    g.setColour (ycolor.black); 
    g.fillPath (pointer);

    // 给指针加一个细黑边，使其更有立体感
    g.setColour (ycolor.darkGrey);
    g.strokePath (pointer, juce::PathStrokeType (1.0f));
}

void YLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if(!slider.isVertical()) return;//如果不是竖状滑块就返回

    float centreX = x + width * 0.5f;
    float radius = width * 0.5f;//交界线的圆弧

    minSliderPos = y + height;
    maxSliderPos = y;

    //---------------------------------------------------------------------------------------
    //先绘制背景框
    juce::Path background;
    background.startNewSubPath(x,y+height);
    background.lineTo(x+width,y+height);
    background.lineTo(x+width,y+radius);
    background.addCentredArc(
        centreX, 
        y+radius, 
        radius, 
        radius, 
        0.0f, //椭圆自身的倾斜角度
        half_pi, //顺时针（Clockwise）方向为正方向
        -half_pi
    );
    background.closeSubPath();
    g.setColour(ycolor.midGrey);
    g.fillPath(background);
    // g.setColour(ycolor.black);
    // g.strokePath(background, PathStrokeType(2.0f));

    float arcBeginY = sliderPos + radius;//圆弧的y轴起点，在边框上

    juce::Path hadWalkedPath;
    hadWalkedPath.startNewSubPath(x,y+height);
    hadWalkedPath.lineTo(x+width,y+height);

    if(arcBeginY > y+height){
        hadWalkedPath.addCentredArc(
            centreX, 
            y+height, 
            radius, 
            minSliderPos - sliderPos, 
            0.0f, //椭圆自身的倾斜角度
            half_pi, //顺时针（Clockwise）方向为正方向
            -half_pi
        );
        hadWalkedPath.closeSubPath();
    }else{
        hadWalkedPath.lineTo(x+width,arcBeginY);
        hadWalkedPath.addCentredArc(
            centreX, 
            arcBeginY, 
            radius, 
            radius, 
            0.0f, //椭圆自身的倾斜角度
            half_pi, //顺时针（Clockwise）方向为正方向
            -half_pi
        );
        hadWalkedPath.closeSubPath();
    }
    g.setColour(ycolor.darkGrey);
    g.fillPath(hadWalkedPath);
}

YLabel::YLabel(juce::String text) 
{
    // 在构造函数里设置默认样式
    // 使用最新的 FontOptions (JUCE 8+)
    setFont (juce::FontOptions (18.0f));
    
    setColour (juce::Label::textColourId, juce::Colours::black);
    setJustificationType (juce::Justification::centred);
    setText(text, juce::dontSendNotification);
    
}
littleLabel::littleLabel(juce::String text){
    setFont (juce::FontOptions (14.0f));
    
    setColour (juce::Label::textColourId, juce::Colours::darkgrey);
    setJustificationType (juce::Justification::centred);
    setText(text, juce::dontSendNotification);
}//字号小一点的标签

YSlider::YSlider(){
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
}

void PlayStopButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) 
{

    g.setColour(ycolor.midGrey);
    if(shouldDrawButtonAsHighlighted) g.fillAll();
    g.setColour(ycolor.darkGrey);
    if(shouldDrawButtonAsDown) g.fillAll();

    // 1. 获取按钮区域，并向内缩进一点，防止图形贴边
    auto bounds = getLocalBounds().toFloat().reduced (getWidth() / 4.0f);
    
    // 2. 设置线条颜色和粗细
    g.setColour (juce::Colours::black);
    const float lineThickness = 3.0f;

    // 3. 根据按钮的开关状态（ToggleState）绘制不同的形状
    if (getToggleState())
    {
        // 状态为 true (开启/运行中)：绘制【停止方块】
        g.drawRect (bounds, lineThickness);
    }
    else
    {
        // 状态为 false (关闭/停止中)：绘制【播放三角】
        juce::Path triangle;
        triangle.startNewSubPath (bounds.getX(), bounds.getY());             // 左上角
        triangle.lineTo (bounds.getRight(), bounds.getCentreY());            // 右侧中心点
        triangle.lineTo (bounds.getX(), bounds.getBottom());                 // 左下角
        triangle.closeSubPath();                                             // 闭合路径

        g.strokePath (triangle, juce::PathStrokeType (lineThickness));
    }


}

rotarySlider::rotarySlider(){

    setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 30);
}

verticalSlider::verticalSlider(){
    setSliderStyle(juce::Slider::LinearVertical);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 30);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
    setColour(juce::Slider::textBoxHighlightColourId, juce::Colours::transparentWhite);
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
}

yTextButton::yTextButton(juce::String initText)
{
    setButtonText(initText);
    setClickingTogglesState(false);
    
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

yComboBox::yComboBox(){
    setColour(juce::ComboBox::arrowColourId, ycolor.black);
    setColour(juce::ComboBox::backgroundColourId, ycolor.midGrey);
    setColour(juce::ComboBox::textColourId, ycolor.black);

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

    g.setColour(ycolor.shallowGrey); // 替换为你的颜色
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

    g.setColour(ycolor.shallowGrey); // 替换为你的颜色
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