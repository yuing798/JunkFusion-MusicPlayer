#include "lookandfeel.hpp"
#include "./otherComponent.hpp"

YLookAndFeel::YLookAndFeel(){
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

    g.setColour (ycolor.black);
    g.setFont (16.0f);
    g.drawFittedText (
        text, 
        bounds.reduced(5.0f).toNearestInt(), 
        juce::Justification::centred, 
        5
    );
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