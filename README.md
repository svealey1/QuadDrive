# Emulsion

A four-stage drive and saturation audio plugin with parallel XY-blend processing.

## Features

- Four independent drive/saturation stages (Hard Clip, Soft Clip, Slow Limit, Fast Limit)
- XY pad for intuitive parallel blend control
- Comprehensive visualization (waveform scope, transfer curve, GR meters)
- Advanced true-peak limiting with overshoot suppression
- VST3 and AU formats
- macOS native support

## Building

### Requirements

- CMake 3.15 or higher
- Xcode (macOS)
- JUCE framework (included as submodule)

### Build Instructions

```bash
# Clone with submodules
git clone --recursive https://github.com/svealey1/Emulsion.git
cd Emulsion

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Build Output

Built plugins will be automatically copied to:
- VST3: `~/Library/Audio/Plug-Ins/VST3/Emulsion.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Emulsion.component`

## Testing

Compatible with:
- Plugin Doctor (for technical analysis)
- Ableton Live
- Logic Pro
- Other major DAWs supporting VST3/AU

## CI/CD

GitHub Actions automatically builds and tests on every push. Tagged releases create downloadable artifacts.

## License

Copyright © 2025 Steve Vealey. All rights reserved.

## Version

Current version: 1.8.7

See [CHANGELOG.md](CHANGELOG.md) for version history.
