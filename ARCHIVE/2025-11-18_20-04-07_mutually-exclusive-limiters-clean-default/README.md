# QuadDrive Backup - 2025-11-18 20:04:07

## Version: Mutually Exclusive Limiters + Clean Default

### Key Features in This Build:

1. **Mutually Exclusive Limiter Buttons**
   - NO OVERSHOOT and TRUE PEAK buttons now toggle each other off
   - Clicking one automatically disables the other
   - Only one limiter can be active at a time (not in series)
   - If both are enabled programmatically, True Peak takes priority

2. **Overshoot Blend Defaults to Clean**
   - Default value: 0.0 (Clean/Gentle character)
   - Was previously: 0.5 (Balanced)
   - New sessions start with gentler overshoot limiting

3. **THRESHOLD Parameter Separated from CALIB_LEVEL**
   - CALIB_LEVEL: Only for normalization/gain matching (0 to -60 dBFS)
   - THRESHOLD: Controls processor ceiling (-12 to 0 dBFS)
   - All 4 drive processors (HC, SC, SL, FL) use THRESHOLD parameter
   - Allows intentional distortion creation for blending

4. **Zero Latency Processing**
   - totalLatencySamples = 0 (no delay compensation)
   - Plugin reports 0 latency to DAW
   - Main Delta mode passes completely transparently
   - Processors handle their own internal 1-sample lookahead

5. **Independent Delta Modes**
   - DELTA MODE: Main drive processors only (orange button)
   - O/S DELTA: No OverShoot limiter only (purple button)
   - TP DELTA: Disabled due to phase alignment complexity

6. **True Peak Limiter Settings**
   - Fast release: ~20ms (responsive for high frequencies)
   - Slow release: ~80ms (for low frequencies)
   - IRC (Intelligent Release Control) active
   - Original punchy character maintained

### Processing Chain:
```
Input → Gain → 4 Drive Processors (XY Blend) → Mix (Wet/Dry) →
Output Gain → ONE Limiter (NO OVERSHOOT *or* TRUE PEAK) → Output
```

### Files Included:
- Source/ - Complete source code
- Quad-Blend Drive.vst3 - Built plugin (Release configuration)
- README.md - This file

### Build Configuration:
- Built with: CMake Release
- Platform: macOS (darwin 24.6.0)
- JUCE Framework: dsp oversampling with 8x linear-phase FIR
- Date: 2025-11-18
- Time: 20:04:07

### Changes from Previous Build (19:44:19):
- Added mutually exclusive button behavior in UI
- Changed OVERSHOOT_BLEND default from 0.5 to 0.0
- Reverted from series limiter processing back to one-or-the-other
- Restored original True Peak timing (was temporarily slower for testing)
