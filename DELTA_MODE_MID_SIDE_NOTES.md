# Delta Mode with Mid-Side Processing - Technical Notes

## Overview
When Delta Mode is engaged while using Mid-Side (M/S) channel mode, the output sounds strange because you're hearing the Mid and Side artifacts separately, which encode different spatial characteristics.

## Processing Order

1. **M/S Encoding** (Line ~1410 in PluginProcessor.cpp)
   - Happens early in the signal chain
   - Converts L/R to M/S: `Mid = (L+R)/2`, `Side = (L-R)/2`

2. **Drive Processing**
   - All saturation/limiting happens in M/S domain when M/S mode is active
   - Each channel (M and S) is processed independently based on Channel Link setting

3. **Delta Computation** (Line ~1928 in PluginProcessor.cpp)
   - `delta = wet_processed - dry_compensated`
   - This happens BEFORE M/S decoding
   - So delta signal is still in M/S domain

4. **M/S Decoding** (Line ~2028 in PluginProcessor.cpp)
   - Converts back to L/R: `Left = Mid + Side`, `Right = Mid - Side`
   - This includes the delta signal if Delta Mode is active

## Why It Sounds Strange

When you engage Delta Mode with Mid-Side:
- You're hearing `(processed_mid - dry_mid)` and `(processed_side - dry_side)`
- Mid channel contains center information (sum of L+R)
- Side channel contains stereo width information (difference of L-R)
- The artifacts have different frequency content and spatial characteristics
- When decoded back to L/R, these M/S artifacts create unusual stereo imaging

## Expected Behavior

This is technically correct behavior - Delta Mode shows only the processing artifacts in whatever channel mode is active:
- **Stereo Mode**: Shows L/R artifacts
- **Mid-Side Mode**: Shows M/S artifacts

## Recommendation

If Delta Mode sounds strange in Mid-Side mode, it's because you're hearing the spatial encoding of the artifacts. This is working as designed, but can be confusing. Users should:
- Use Delta Mode primarily in Stereo mode for more intuitive artifact monitoring
- Understand that M/S Delta Mode shows how the center vs sides are being affected differently
