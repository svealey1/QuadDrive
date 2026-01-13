#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * @brief Professional mastering-grade look and feel for S.T.E.V.E.
 * Inspired by Shadow Hills and iZotope aesthetics
 */
class STEVELookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Color palette
    static constexpr uint32_t backgroundDeep    = 0xff0a0a0a;  // Deep black
    static constexpr uint32_t backgroundPanel   = 0xff1a1a1a;  // Panel background
    static constexpr uint32_t backgroundControl = 0xff2a2a2a;  // Control background
    static constexpr uint32_t textPrimary       = 0xffd0d0d0;  // Primary text
    static constexpr uint32_t textSecondary     = 0xffc0c0c0;  // Secondary text
    static constexpr uint32_t accentBlue        = 0xff4a9eff;  // Accent blue
    static constexpr uint32_t warningAmber      = 0xffffaa00;  // Warning amber
    static constexpr uint32_t criticalRed       = 0xffff4040;  // Critical red
    static constexpr uint32_t outline           = 0xff404040;  // Outlines
    static constexpr uint32_t outlineSubtle     = 0xff303030;  // Subtle outlines

    STEVELookAndFeel()
    {
        // Window styling
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(backgroundDeep));
        setColour(juce::DocumentWindow::textColourId, juce::Colour(textPrimary));

        // ComboBox styling
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(backgroundPanel));
        setColour(juce::ComboBox::textColourId, juce::Colour(textPrimary));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(outline));
        setColour(juce::ComboBox::buttonColourId, juce::Colour(backgroundControl));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff808080));

        // PopupMenu styling
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(backgroundPanel));
        setColour(juce::PopupMenu::textColourId, juce::Colour(textPrimary));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3a3a3a));

        // Label styling
        setColour(juce::Label::textColourId, juce::Colour(textSecondary));

        // Slider styling
        setColour(juce::Slider::thumbColourId, juce::Colour(accentBlue));
        setColour(juce::Slider::trackColourId, juce::Colour(outline));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(textPrimary));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(backgroundPanel));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(outlineSubtle));

        // TextButton styling
        setColour(juce::TextButton::buttonColourId, juce::Colour(backgroundControl));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(accentBlue));
        setColour(juce::TextButton::textColourOffId, juce::Colour(textPrimary));
        setColour(juce::TextButton::textColourOnId, juce::Colour(backgroundDeep));

        // ToggleButton styling
        setColour(juce::ToggleButton::textColourId, juce::Colour(textPrimary));
        setColour(juce::ToggleButton::tickColourId, juce::Colour(accentBlue));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override
    {
        const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 8.0f;
        const float centreX = x + width * 0.5f;
        const float centreY = y + height * 0.5f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Outer ring (track)
        juce::Path trackPath;
        trackPath.addCentredArc(centreX, centreY, radius, radius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);

        g.setColour(juce::Colour(0xff202020));
        juce::PathStrokeType strokeType(3.0f);
        g.strokePath(trackPath, strokeType);

        // Value arc
        juce::Path valuePath;
        valuePath.addCentredArc(centreX, centreY, radius, radius,
                               0.0f, rotaryStartAngle, angle, true);

        g.setColour(juce::Colour(accentBlue));
        strokeType = juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        g.strokePath(valuePath, strokeType);

        // Knob center
        const float knobRadius = radius * 0.6f;

        // Knob base (gradient)
        juce::ColourGradient gradient(juce::Colour(backgroundControl), centreX, centreY - knobRadius,
                                      juce::Colour(backgroundPanel), centreX, centreY + knobRadius, false);
        g.setGradientFill(gradient);
        g.fillEllipse(centreX - knobRadius, centreY - knobRadius,
                     knobRadius * 2.0f, knobRadius * 2.0f);

        // Knob outline
        g.setColour(juce::Colour(outline));
        g.drawEllipse(centreX - knobRadius, centreY - knobRadius,
                     knobRadius * 2.0f, knobRadius * 2.0f, 1.5f);

        // Pointer
        juce::Path pointer;
        const float pointerLength = knobRadius * 0.7f;
        const float pointerThickness = 2.5f;

        pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.8f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        g.setColour(juce::Colour(textPrimary));
        g.fillPath(pointer);

        // Center dot
        g.fillEllipse(centreX - 2.0f, centreY - 2.0f, 4.0f, 4.0f);

        // Tick marks
        g.setColour(juce::Colour(0xff505050));
        const int numTicks = 11;
        for (int i = 0; i < numTicks; ++i)
        {
            const float tickAngle = rotaryStartAngle + (i / (float)(numTicks - 1)) * (rotaryEndAngle - rotaryStartAngle);
            const float tickLength = 4.0f;
            const float x1 = centreX + (radius + 4.0f) * std::cos(tickAngle - juce::MathConstants<float>::halfPi);
            const float y1 = centreY + (radius + 4.0f) * std::sin(tickAngle - juce::MathConstants<float>::halfPi);
            const float x2 = centreX + (radius + 4.0f + tickLength) * std::cos(tickAngle - juce::MathConstants<float>::halfPi);
            const float y2 = centreY + (radius + 4.0f + tickLength) * std::sin(tickAngle - juce::MathConstants<float>::halfPi);

            g.drawLine(x1, y1, x2, y2, 1.0f);
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearVertical)
        {
            const bool isHorizontal = style == juce::Slider::LinearHorizontal;
            const float trackThickness = 4.0f;

            juce::Rectangle<float> track;
            if (isHorizontal)
            {
                track = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y) + height * 0.5f - trackThickness * 0.5f,
                                               static_cast<float>(width), trackThickness);
            }
            else
            {
                track = juce::Rectangle<float>(static_cast<float>(x) + width * 0.5f - trackThickness * 0.5f,
                                               static_cast<float>(y), trackThickness, static_cast<float>(height));
            }

            // Track background
            g.setColour(juce::Colour(0xff202020));
            g.fillRoundedRectangle(track, 2.0f);

            // Value fill
            juce::Rectangle<float> fill;
            if (isHorizontal)
            {
                fill = juce::Rectangle<float>(track.getX(), track.getY(),
                                              sliderPos - track.getX(), track.getHeight());
            }
            else
            {
                fill = juce::Rectangle<float>(track.getX(), sliderPos,
                                              track.getWidth(), track.getBottom() - sliderPos);
            }

            g.setColour(juce::Colour(accentBlue));
            g.fillRoundedRectangle(fill, 2.0f);

            // Thumb
            const float thumbSize = 14.0f;
            juce::Point<float> thumbPos;
            if (isHorizontal)
                thumbPos = { sliderPos, track.getCentreY() };
            else
                thumbPos = { track.getCentreX(), sliderPos };

            g.setColour(juce::Colour(textPrimary));
            g.fillEllipse(thumbPos.x - thumbSize * 0.5f, thumbPos.y - thumbSize * 0.5f,
                         thumbSize, thumbSize);

            g.setColour(juce::Colour(outline));
            g.drawEllipse(thumbPos.x - thumbSize * 0.5f, thumbPos.y - thumbSize * 0.5f,
                         thumbSize, thumbSize, 1.5f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

        // Background
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 3.0f);

        // Outline
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

        // Arrow
        auto arrowZone = juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH).toFloat();
        juce::Path path;
        path.startNewSubPath(arrowZone.getX() + 3.0f, arrowZone.getCentreY() - 2.0f);
        path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 2.0f);
        path.lineTo(arrowZone.getRight() - 3.0f, arrowZone.getCentreY() - 2.0f);

        g.setColour(box.findColour(juce::ComboBox::arrowColourId));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);

        juce::Colour baseColour = backgroundColour;

        if (shouldDrawButtonAsDown)
            baseColour = baseColour.brighter(0.1f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.05f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(juce::Colour(outline));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsDown);
        auto bounds = button.getLocalBounds().toFloat();
        auto buttonText = button.getButtonText();

        // Get the button's tick color - check if a custom color was set on this specific button
        // If yes, use it; otherwise fall back to default accent blue
        juce::Colour tickColour = button.isColourSpecified(juce::ToggleButton::tickColourId)
            ? button.findColour(juce::ToggleButton::tickColourId)
            : juce::Colour(accentBlue);

        juce::Colour offColour = button.isColourSpecified(juce::ToggleButton::tickDisabledColourId)
            ? button.findColour(juce::ToggleButton::tickDisabledColourId)
            : juce::Colour(backgroundPanel);

        // Check if this is a single-letter M or S button (mute/solo style)
        bool isMuteOrSoloButton = (buttonText == "M" || buttonText == "S");

        if (isMuteOrSoloButton)
        {
            // Draw as a filled button with the letter centered
            auto buttonBounds = bounds.reduced(1.0f);

            // Background - clearly different between ON and OFF states
            if (button.getToggleState())
            {
                // ON: Fully filled with the tick color
                g.setColour(tickColour);
                g.fillRoundedRectangle(buttonBounds, 3.0f);
            }
            else
            {
                // OFF: Dark transparent background - clearly "empty" looking
                g.setColour(juce::Colour(0x20ffffff));  // Very subtle white tint
                g.fillRoundedRectangle(buttonBounds, 3.0f);

                // Subtle highlight on hover
                if (shouldDrawButtonAsHighlighted)
                {
                    g.setColour(tickColour.withAlpha(0.2f));
                    g.fillRoundedRectangle(buttonBounds, 3.0f);
                }
            }

            // Outline - more prominent when OFF to show button boundary
            if (button.getToggleState())
                g.setColour(tickColour.brighter(0.2f));
            else if (shouldDrawButtonAsHighlighted)
                g.setColour(tickColour.withAlpha(0.6f));
            else
                g.setColour(juce::Colour(0xff505050));  // Brighter outline when OFF
            g.drawRoundedRectangle(buttonBounds, 3.0f, 1.0f);

            // Text - white/bright on active, dimmed on inactive
            if (button.getToggleState())
                g.setColour(juce::Colours::white);
            else
                g.setColour(juce::Colour(0xff808080));  // Gray when OFF

            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(buttonText, buttonBounds, juce::Justification::centred);
        }
        else
        {
            // Standard checkbox-style toggle for other toggle buttons
            const float tickSize = juce::jmin(16.0f, bounds.getHeight() - 4.0f);

            // Checkbox bounds
            juce::Rectangle<float> tickBounds(bounds.getX() + 2.0f, bounds.getCentreY() - tickSize * 0.5f,
                                              tickSize, tickSize);

            // Background - use tick color when on for filled button effect
            if (button.getToggleState())
            {
                g.setColour(tickColour.withAlpha(0.2f));
                g.fillRoundedRectangle(tickBounds, 3.0f);
            }
            else
            {
                g.setColour(juce::Colour(backgroundPanel));
                g.fillRoundedRectangle(tickBounds, 3.0f);
            }

            // Outline - use tick color when on or highlighted
            if (button.getToggleState())
                g.setColour(tickColour);
            else if (shouldDrawButtonAsHighlighted)
                g.setColour(tickColour.withAlpha(0.6f));
            else
                g.setColour(juce::Colour(outline));
            g.drawRoundedRectangle(tickBounds, 3.0f, 1.0f);

            // Checkmark - use button's tick color
            if (button.getToggleState())
            {
                g.setColour(tickColour);
                juce::Path tick;
                tick.startNewSubPath(tickBounds.getX() + tickSize * 0.2f, tickBounds.getCentreY());
                tick.lineTo(tickBounds.getX() + tickSize * 0.4f, tickBounds.getBottom() - tickSize * 0.25f);
                tick.lineTo(tickBounds.getRight() - tickSize * 0.2f, tickBounds.getY() + tickSize * 0.25f);
                g.strokePath(tick, juce::PathStrokeType(2.5f));
            }

            // Label - use tick color when on
            g.setColour(button.getToggleState() ? tickColour : juce::Colour(textPrimary));
            g.drawFittedText(buttonText,
                            juce::Rectangle<int>(static_cast<int>(tickBounds.getRight() + 6.0f),
                                                static_cast<int>(bounds.getY()),
                                                static_cast<int>(bounds.getWidth() - tickBounds.getRight() - 6.0f),
                                                static_cast<int>(bounds.getHeight())),
                            juce::Justification::centredLeft, 1);
        }
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox(slider);

        label->setColour(juce::Label::textColourId, juce::Colour(textPrimary));
        label->setColour(juce::Label::backgroundColourId, juce::Colour(backgroundPanel));
        label->setColour(juce::Label::outlineColourId, juce::Colour(outlineSubtle));
        label->setFont(juce::FontOptions(13.0f, juce::Font::plain));
        label->setJustificationType(juce::Justification::centred);

        return label;
    }

    // Helper method to get processor colors for GR visualization
    static juce::Colour getProcessorColor(int processorIndex, bool multiColorMode = true)
    {
        if (multiColorMode)
        {
            switch (processorIndex)
            {
                case 0: return juce::Colour(0xffff5050);  // Hard Clip - red
                case 1: return juce::Colour(0xffff963c);  // Soft Clip - orange
                case 2: return juce::Colour(0xffffdc50);  // Slow Limit - yellow
                case 3: return juce::Colour(0xff50a0ff);  // Fast Limit - blue
                default: return juce::Colour(accentBlue);
            }
        }
        else
        {
            return juce::Colour(accentBlue);  // Unified accent color
        }
    }
};
