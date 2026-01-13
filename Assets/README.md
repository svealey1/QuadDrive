# S.T.E.V.E. SVG Assets for Figma

This directory contains SVG assets for designing the S.T.E.V.E. (Signal Transfer Enhanced Volume Engine) plugin UI in Figma.

## Color Palette

### Background Colors
- **Main Background**: `#0A0A0F` - Darkest background
- **Panel Background**: `#1A1A1F` - Component backgrounds
- **Elevated Elements**: `#23232A` - Panels, dialogs
- **Controls**: `#2D2D34` / `#2D2D35` - Buttons, knobs

### Accent Colors
- **Primary Accent (Blue)**: `#4FC3F7` - Active elements, value indicators
- **Warning (Orange)**: `#FF8C3C` - Active/saved states, calibration indicator
- **Success (Green)**: `#00E676` - Meter safe zone
- **Warning (Yellow)**: `#FFD600` - Meter warning zone
- **Danger (Red)**: `#FF1744` - Meter clip zone, delta mode

### Text Colors
- **Primary Text**: `#FFFFFF` at 85-100% opacity
- **Secondary Text**: `#B4B4BE` - Labels, subtitles
- **Borders**: `#3C3C41` - UI borders, dividers

## Asset List

1. **STEVE_Logo.svg** - Main logo with title and subtitle
2. **XY_Pad_Grid.svg** - XY pad grid with corner labels
3. **Knob_Base.svg** - Rotary knob template
4. **Button_Normal.svg** - Default button state
5. **Button_Active.svg** - Active/lit button state (orange)
6. **Meter_Bar.svg** - Vertical meter with gradient
7. **Waveform_Scope.svg** - STEVEScope waveform display
8. **Slider_Horizontal.svg** - Horizontal slider component
9. **Toggle_MS_Buttons.svg** - M/S toggle button pair
10. **Panel_Background.svg** - Advanced/Calibration panel background
11. **Icon_Link.svg** - Link icon for threshold/gain linking

## Usage in Figma

1. Import SVGs directly into Figma
2. Use as components or convert to Figma components
3. Customize colors using the palette above
4. Scale proportionally to maintain design consistency

## Typography

- **Logo**: Arial Bold, 72pt
- **Subtitle**: Arial Regular, 18pt
- **Button Text**: Arial Bold, 12pt
- **Labels**: Arial Regular, 10-12pt
- **Meters**: Arial Bold, 7-9pt

## Layout Grid

- **Window Size (Collapsed)**: 950 × 600px
- **Window Size (Expanded)**: 950 × 900px
- **Padding**: 20px
- **Component Spacing**: 10-12px
- **XY Pad**: Square, centered
- **Scope Height**: ~380px when visible

## Component States

### Buttons
- **Normal**: Dark gray background (#2D2D34)
- **Active/Saved**: Orange background (#FF8C3C)
- **Hover**: Slight brightness increase
- **Disabled**: 50% opacity

### Meters
- **Safe**: Green (#00E676) - Below -6dB
- **Warning**: Yellow (#FFD600) - -6dB to -3dB
- **Hot**: Orange (#FF6B00) - -3dB to 0dB
- **Clip**: Red (#FF1744) - Above 0dB

### Knobs
- **Track**: Dark gray (#2D2D35)
- **Value Indicator**: Blue (#4FC3F7)
- **Range**: 270° rotation typical

## Export Settings

- **Format**: SVG
- **Precision**: 2 decimal places
- **Include**: viewBox attribute
- **Optimize**: Remove unnecessary metadata
