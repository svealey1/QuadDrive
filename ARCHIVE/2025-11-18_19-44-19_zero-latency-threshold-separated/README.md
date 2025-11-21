# QuadDrive Backup - 2025-11-18 19:44:19

## Version: Zero Latency + Threshold Separated

### Key Features in This Build:

1. **THRESHOLD Parameter Separated from CALIB_LEVEL**
   - CALIB_LEVEL: Now only used for normalization/gain matching (0 to -60 dBFS)
   - THRESHOLD: New parameter for controlling processor ceiling (-12 to 0 dBFS)
   - All 4 drive processors (HC, SC, SL, FL) use THRESHOLD parameter
   - Allows intentional distortion creation for blending with mix knob

2. **Zero Latency Processing**
   - totalLatencySamples = 0 (no delay compensation)
   - Plugin reports 0 latency to DAW
   - Main Delta mode now passes completely transparently
   - Processors handle their own internal 1-sample lookahead

3. **Independent Delta Modes**
   - DELTA MODE: Main drive processors only (orange button)
   - O/S DELTA: No OverShoot limiter only (purple button)
   - TP DELTA: Disabled due to phase alignment complexity

4. **Bug Fixes**
   - Fixed main Delta mode phasing issues
   - Fixed false gain reduction display
   - Corrected dry signal delay compensation

### Files Included:
- Source/ - Complete source code
- Quad-Blend Drive.vst3 - Built plugin (Release configuration)
- README.md - This file

### Build Configuration:
- Built with: CMake Release
- Platform: macOS (darwin 24.6.0)
- JUCE Framework: dsp oversampling with 8x linear-phase FIR
- Date: 2025-11-18
- Time: 19:44:19
