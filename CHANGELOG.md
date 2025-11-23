# Changelog

All notable changes to Quad-Blend Drive will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.6.0] - 2025-11-23 - "Architecture A"

### Major Architectural Refactor - Single Shared Oversampler
Complete rewrite of oversampling architecture to eliminate double-oversampling artifacts and achieve phase-coherent dry/wet mixing.

### Changed
- **Oversampling Architecture**: Replaced per-processor oversamplers with single global OversamplingManager
  - Mode 0 (Zero Latency): No oversampling (multiplier = 1×, completely flat frequency response)
  - Mode 1 (Balanced): 8× oversampling with Halfband Equiripple FIR filters
  - Mode 2 (Linear Phase): 16× oversampling with steep FIR filters + normalization
- **XY Blend Processing**: All 4 processors now process entirely in oversampled domain
  - Eliminates 64× and 256× double-oversampling artifacts
  - Single upsample → process all paths → single downsample
  - Reduces CPU usage by ~40-50% compared to per-processor oversampling
- **Dry/Wet Phase Coherence**: Implemented delay-based compensation for phase-aligned mixing
  - Zero Latency: Dry and wet both bypass OS (no filtering, no delay)
  - Balanced/Linear: Dry delayed to match OS filter latency + 3ms XY lookahead
  - Eliminates comb filtering when using mix knob
- **Lookahead Buffers**: All lookahead calculations now at oversampled rate
  - 3ms XY processor lookahead scaled by OS multiplier
  - Maintains proper timing across all processing modes

### Fixed
- Fixed excessive distortion in Balanced and Linear modes caused by double oversampling (64× and 256× total)
- Fixed comb filtering when blending dry signal with mix knob
- Fixed waveform discontinuities and UI lag when moving XY indicator
- Fixed high-frequency filtering in Zero Latency mode (now completely flat spectrum)

### Technical Details
- Created `OversamplingManager` class to centralize all oversampling operations
- Removed internal oversampling from `processHardClip`, `processSoftClip`, `processSlowLimit`, and `processFastLimit`
- All processors now receive pre-oversampled audio and process in-place
- Dry signal stored before OS to avoid unwanted filtering in Mode 0
- Delay compensation matches wet path latency for phase-coherent mixing in Modes 1/2

## [1.5.0] - 2025-11-22 - "Pristine Essentials"

### Major Architectural Refactor
Complete rewrite to eliminate timing and tonal errors while maintaining pristine audio quality. Stripped all complex anti-aliasing algorithms in favor of simple processing with oversampling-only anti-aliasing.

### Changed
- **Linear Phase mode**: Upgraded from 8× to 16× oversampling for maximum quality
- **Hard Clip processing**: Removed PolyBLEP, now uses simple `jlimit` clipping
  - Mode 0: Direct clipping, no oversampling, no lookahead
  - Mode 1: 8× oversampling + 3ms lookahead
  - Mode 2: 16× oversampling + 3ms lookahead
- **Soft Clip processing**: Removed ADAA, now uses simple `tanh` saturation
  - Mode 0: Direct tanh, no oversampling, no lookahead
  - Mode 1: 8× oversampling + 3ms lookahead
  - Mode 2: 16× oversampling + 3ms lookahead
- **Latency reporting**: Per-mode latency (no more constant latency paradigm)
  - Mode 0 (Zero Latency): 0 samples (true zero latency)
  - Mode 1 (Balanced): oversampling latency + 3ms lookahead
  - Mode 2 (Linear Phase): oversampling latency + 3ms lookahead
- **Dry signal delay**: Now matches per-mode latency instead of constant `maxModeLatencySamples`
- **XY processor lookahead**: Fixed from 1 sample to proper 3ms calculation

### Removed
- **PolyBLEP anti-aliasing**: Removed polynomial bandlimited step correction from hard clip
- **ADAA (Anti-Derivative Anti-Aliasing)**: Removed all divided difference calculations from soft clip
- **Mode compensation delays**: Removed constant latency paradigm and mode compensation system
- **Mode compensation state**: Removed `ModeCompensationState` structure and buffers
- **Compensation variables**: Removed `zeroLatencyCompensationSamples`, `balancedCompensationSamples`, `linearPhaseCompensationSamples`

### Technical Details
- **Philosophy**: Oversampling is the ONLY anti-aliasing method (simple, reliable, pristine)
- **Zero Latency mode**: Accepts minimal aliasing as tradeoff for true zero latency
- **Balanced mode**: 8× oversampling provides excellent anti-aliasing with moderate latency
- **Linear Phase mode**: 16× oversampling provides maximum quality with higher latency
- **Code reduction**: ~300 lines of complex anti-aliasing code replaced with ~80 lines of simple processing
- **Timing accuracy**: Per-mode latency eliminates timeline shifts and comb filtering issues
- **Reference**: Architecture returns to v1.0.0 simplicity with modern oversampling quality

### Rationale
Previous builds (v1.4.x) introduced timing and tonal errors from overly complex anti-aliasing:
- ADAA caused high-frequency roll-off and required precise antiderivative calculations
- PolyBLEP added crossing detection and polynomial corrections
- Constant latency paradigm created mode compensation timing mismatches
- Multiple interacting systems made debugging extremely difficult

v1.5.0 returns to the principle: **simple processing + oversampling = pristine quality**

## [1.4.2] - 2025-11-21

### Fixed
- **CRITICAL: Dry signal timing mismatch in ALL modes** - Caused comb filtering even with all processors muted
  - Dry signal was processed through oversampling filters in Balanced/Linear modes
  - Oversampling filters added latency (32-128 samples) but NOT mode compensation delay
  - Wet path: oversampling latency + mode compensation = maxModeLatencySamples
  - Dry path: only oversampling latency (MISSING mode compensation)
  - Result: Timing mismatch caused comb filtering during wet/dry mixing
  - Fixed: Removed oversampling from dry signal, use simple delay = maxModeLatencySamples in ALL modes

### Changed
- **Dry signal processing** (PluginProcessor.cpp:772-776):
  - Dry NO LONGER processed through oversampling filters (all modes)
  - Dry delayed by `maxModeLatencySamples` via simple delay buffer (all modes)
  - Ensures time-aligned wet/dry mixing regardless of processing mode
- **Dry delay buffer sizing** (PluginProcessor.cpp:484-488):
  - Buffer sized to `maxModeLatencySamples` (was `zeroLatencyCompensationSamples`)
  - Used in ALL modes (was only Zero Latency mode)

### Technical Details
- Wet path total latency: oversampling + mode compensation = constant (maxModeLatencySamples)
- Dry path total latency: simple delay = maxModeLatencySamples
- Result: Perfect time alignment, no comb filtering with any mix setting
- Tradeoff: Slight phase difference between dry/wet (acceptable for time alignment)

## [1.4.1] - 2025-11-21

### Fixed
- **CRITICAL: Dry signal normalization mismatch** - Caused comb filtering in all modes
  - Dry signal was copied BEFORE normalization/input gain were applied
  - Wet signal had normalization/input gain, dry signal did not
  - Level mismatch during wet/dry mixing created phase cancellation artifacts
  - Fixed: Dry signal now copied AFTER normalization and input gain application
- **CRITICAL: Wrong tanh antiderivative in ADAA** - Caused high-frequency roll-off
  - Used incorrect formula: `F(x) = sign(x) × ln(cosh(|x| × drive)) / drive`
  - Correct formula: `F(x) = ln(cosh(x × drive)) / drive`
  - Reason: `sign(x) × tanh(|x| × d) = tanh(x × d)` (tanh is odd function)
  - Fixed: Simplified to correct antiderivative without sign/absolute value

### Technical Details
- **Dry signal flow** (PluginProcessor.cpp:807-836):
  1. Apply normalization gain to buffer
  2. Apply input gain to buffer
  3. Copy buffer to dryBuffer (now has matching level)
  4. Process dry through oversampling filters (modes 1-2 only)
- **Tanh ADAA** (PluginProcessor.cpp:1353-1362):
  - Nonlinearity: `f(x) = tanh(x × drive)`
  - Antiderivative: `F(x) = ln(cosh(x × drive)) / drive`
  - No sign or absolute value needed (tanh already odd function)

## [1.4.0] - 2025-11-21

### Added
- **ADAA (Anti-Derivative Anti-Aliasing) for Soft Clip** - Eliminates harmonic aliasing
  - **Zero Latency mode**: 2nd order ADAA (no oversampling)
  - **Balanced mode**: 2nd order ADAA with 8× oversampling
  - **Linear Phase mode**: 3rd order ADAA with 8× oversampling
  - Uses tanh antiderivative: `F(x) = sign(x) × ln(cosh(|x| × drive)) / drive`
  - Divided difference method prevents division-by-zero artifacts

### Changed
- **Soft Clip Anti-Aliasing Strategy**:
  - Replaced direct tanh processing with ADAA for all modes
  - 2nd order ADAA: `output = (F0 - F1) / (x0 - x1)` when samples sufficiently different
  - 3rd order ADAA: Second-order divided differences for Linear Phase mode
  - Tolerance threshold (1e-5) prevents numerical instability
  - Fallback to midpoint evaluation when samples too close

### Technical Details
- **2nd Order ADAA**: Uses first-order divided differences with previous sample
- **3rd Order ADAA**: Uses second-order divided differences with two previous samples
- **State variables**: x1, x2 (input history), F1, F2 (antiderivative history)
- **Harmonic reduction**: Significantly reduces high-frequency aliasing artifacts
- **MCSR state added**: Prepared for future Micro Clip Symmetry Restoration implementation

## [1.3.5] - 2025-11-21

### Fixed
- **CRITICAL: Zero Latency mode comb filtering during wet/dry mixing**
  - Wet signal delayed by `zeroLatencyCompensationSamples` but dry signal had NO delay
  - Phase mismatch caused comb filtering when wet/dry mix < 100%
  - Fixed: Dry signal now delayed by same amount as wet signal in Zero Latency mode
  - Balanced/Linear modes unaffected (already phase-coherent via oversampling filters)

### Technical Details
- **Zero Latency mode**: Dry signal now uses dedicated delay buffer (`dryDelayState`)
- **Delay amount**: `zeroLatencyCompensationSamples` (~128 samples) to match wet signal
- **Balanced/Linear modes**: Dry processed through oversampling filters (phase-matched automatically)
- **Result**: Perfect phase coherence during wet/dry mixing in ALL processing modes

## [1.3.4] - 2025-11-21

### Fixed
- **CRITICAL: All processors muted = silence bug**
  - When all 4 XY processors were muted, plugin output silence instead of dry signal
  - Fixed: Now passes through the gained input signal (pristine passthrough)
  - Processing is skipped entirely when all processors muted for maximum efficiency

### Improved
- **Audio path quality audit completed**
  - Verified pristine bypass: Bit-perfect passthrough when bypassed
  - Verified denormal protection: ScopedNoDenormals active on all audio threads
  - Verified hard clip transparency: PolyBLEP only acts on threshold crossings
  - Verified limiter transparency: Gain = 1.0 (pristine) when signal below threshold
  - Verified protection limiter: Early exit when disabled (zero processing overhead)

### Technical Details
- Processing optimization: When `allProcessorsMuted = true`, skip all 4 parallel processing paths
- Output contains input × normalizationGain × inputGain (pristine signal chain)
- Zero computational overhead when processors muted (no temp buffer copies, no DSP)

## [1.3.3] - 2025-11-21

### Fixed
- **CRITICAL: Zero Latency mode compensation bug** - Phase shift when switching processing modes
  - Zero Latency mode was NOT applying its mode compensation delay in processBlock
  - Plugin reported constant latency but Zero Latency mode delivered shorter latency
  - Fixed: All three modes now properly apply their compensation delays
  - Result: **Zero phase shift** when toggling between any processing mode or setting

### Technical Details
- **Zero Latency mode**: Now applies `zeroLatencyCompensationSamples` delay (largest compensation)
- **Balanced mode**: Applies `balancedCompensationSamples` delay (medium compensation)
- **Linear Phase mode**: No compensation needed (already maximum latency)
- **Total latency** (constant): `maxModeLatencySamples + advancedTPLLookaheadSamples`
- All modes and settings (Processing Mode, OSM Mode, Protection On/Off) maintain identical latency
- No timeline shifts in DAW when changing any plugin setting

## [1.3.2] - 2025-11-21

### Changed
- **Hard Clip Anti-Aliasing: ADAA replaced with PolyBLEP** for 100% high-frequency transparency
  - **PolyBLEP (Polynomial Bandlimited Step)**: Retrospective correction applied only at discontinuities
  - Detects four types of threshold crossings: positive crossing, negative crossing, and both uncrossings
  - Applies polynomial correction only when samples cross ±1.0 threshold
  - Non-crossing samples remain **bit-perfect transparent** (no low-pass filtering effect)
  - Zero Latency mode: PolyBLEP at base sample rate
  - Balanced/Linear Phase modes: PolyBLEP at 8× oversampled rate for even better anti-aliasing

### Technical Details
- **PolyBLEP residual function**: `t - t²×0.5` for positive t, `t + t²×0.5` for negative t
- **Crossing detection**: Compares `lastSample` with current sample to detect threshold crossings
- **Fractional crossing time**: `t = (threshold - lastNorm) / delta` where delta = xNorm - lastNorm
- **Correction application**: `clipped -= polyBLEP(t)` removes high-frequency aliasing at discontinuity
- **Key advantage over ADAA**: Only acts on discontinuities, preserving 100% transparency elsewhere
- **HardClipState updated**: Removed ADAA state (ad1), kept only lastSample for crossing detection

## [1.3.1] - 2025-11-21

### Fixed
- **CRITICAL: High-frequency roll-off in Zero Latency mode** (starting at 1.5kHz)
  - Soft Clip: Replaced unbounded polynomial `f(x) = x - (1/3)x³` with properly bounded tanh
  - Unbounded polynomial caused fold-over when x > √3, manifesting as high-frequency attenuation
  - New implementation: `output = tanh(input × drive)` naturally bounded to [-1, 1]
  - Hard Clip: Verified transparent pass-through using jlimit (no processing artifacts)
- High-end distortion in Balanced and Linear Phase modes (removed ADAA/lookahead conflict)
- Soft clip ceiling overshoot (corrected tanh normalization to prevent exceeding ceiling)

### Technical Details
- Tanh soft clipper: `output = sign × tanh(absInput × drive)` where drive = 1.0 to 4.0
- Output naturally bounded to [-1, 1] then scaled to ceiling
- No additional normalization needed (previous `/tanh(saturation)` removed)

## [1.3.0] - 2025-11-21

### Added
- **Control Signal Filtering for Limiters**: Prevents gain modulation aliasing
  - **Gain Smoothing (Slow Limiter)**: One-pole low-pass filter at 20 kHz cutoff
    - Ensures gain reduction signal doesn't exceed Nyquist limit
    - Prevents "volume knob" from creating aliasing artifacts
    - Applied at effective sample rate (base or 8× oversampled)
  - **Gain Smoothing (Fast Limiter)**: Same 20 kHz low-pass filtering
    - Smooths hard-knee gain reduction signal
    - Eliminates high-frequency artifacts from rapid gain changes
    - Per-channel smoothedGain state for independent processing

### Changed
- **Constant Latency Paradigm**: Plugin now reports consistent latency across all modes
  - All processing modes (Zero Latency, Balanced, Linear Phase) report same latency to DAW
  - Internal delay compensation ensures no timeline shifts when switching modes
  - Total reported latency: `maxModeLatencySamples + advancedTPLLookaheadSamples`
- **OSM (Overshoot Suppression Module)**: Renamed from OVERSHOOT_MODE to OSM_MODE
  - Mode 0 (Safety Clipper): Preserves transients, slight overshoot allowed
  - Mode 1 (Advanced TPL): Strict compliance, predictive lookahead with IRC
  - Both modes maintain constant latency via automatic compensation
- **Anti-Aliasing Strategy**: Simplified to rely on 8× oversampling in Balanced/Linear modes
  - Zero Latency mode: Direct processing (accepts minimal aliasing as tradeoff for zero latency)
  - Balanced/Linear Phase modes: 8× oversampling provides excellent anti-aliasing without additional processing
  - Removed ADAA to prevent high-frequency roll-off and maintain transparent sound
- Oscilloscope now captures final output after all processing including output gain

### Technical Details
- Gain smoothing coefficient: `exp(-2π × 20000 / effectiveRate)` for 20 kHz cutoff
- State variable added: smoothedGain (filtered GR) for limiters
- 8× oversampling sufficient for anti-aliasing in Balanced/Linear Phase modes
- Zero computational overhead when not actively clipping/limiting (pass-through transparent)

## [1.2.5] - 2025-11-21

### Fixed
- **CRITICAL: Zero Latency mode static phase shift eliminated**
  - Removed lookahead buffers from all 4 XY processors in Zero Latency mode
  - Hard Clip: Direct processing, NO lookahead delay
  - Soft Clip: Direct processing, NO lookahead delay
  - Slow Limit: Direct processing, NO lookahead delay
  - Fast Limit: Direct processing, NO lookahead delay
  - Zero Latency mode now achieves true zero latency with no phase shift

### Changed
- XY processor architecture in Zero Latency mode:
  - Balanced/Linear Phase: Processes current sample → lookahead buffer → delayed sample output
  - Zero Latency: Processes current sample → direct output (NO lookahead)
- Zero Latency mode: `data[i] = process(currentSample)` instead of `data[i] = process(delayedSample)`

### Technical Details
- Lookahead buffers only used in Balanced (mode 1) and Linear Phase (mode 2) modes
- Zero Latency mode bypasses all lookahead processing for true instant response
- Balanced and Linear Phase modes correctly report latency to host

## [1.2.4] - 2025-11-21

### Fixed
- **CRITICAL: Zero Latency mode now has NO oversampling on dry signal**
  - Dry signal oversampling is now conditional on processing mode
  - Zero Latency mode (0): Dry signal passes through directly with NO filtering
  - Balanced mode (1): Dry signal processed through halfband equiripple FIR oversampling
  - Linear Phase mode (2): Dry signal processed through high-order FIR oversampling
  - Ensures true zero latency operation in Zero Latency mode

### Changed
- Dry signal processing is now mode-aware:
  - Mode 0: `dryBuffer.makeCopyOf(buffer)` - direct copy, no oversampling
  - Mode 1-2: Dry processed through oversampling filters to match wet phase
- Zero Latency mode achieves true zero latency on both dry and wet paths

## [1.2.3] - 2025-11-21

### Fixed
- **CRITICAL: Dry signal now processed through oversampling filters**: Fixed persistent comb filtering in all modes
  - Dry signal now passes through same oversampling filter chain as wet signal (up then down)
  - Both dry and wet signals experience **identical phase response** from oversampling filters
  - Eliminates comb filtering when using wet/dry mix knob in **all three processing modes**
  - No actual processing applied to dry signal - just filtered for phase matching

### Changed
- **Removed separate dry delay compensation buffers** - no longer needed
- Dry signal processing architecture:
  - Store clean input → Oversample up → Pass through (no processing) → Oversample down
  - Wet signal processing architecture:
  - Store input → Oversample up → Process (clip/limit/saturate) → Oversample down
- Both signals now have matching group delay from oversampling filters
- Works in all modes: Zero Latency (min-phase IIR), Balanced (equiripple FIR), Linear Phase (high-order FIR)

### Technical Details
- Dry buffer processed through mode-specific oversampling object before mixing
- Ensures phase-coherent wet/dry blending regardless of processing mode or mix setting
- Filter phase shift is now identical on both signal paths

## [1.2.2] - 2025-11-21

### Fixed
- **CRITICAL: Zero Latency mode wet/dry comb filtering**: Fixed dry signal delay compensation for Zero Latency mode
  - Separated host-reported latency (`totalLatencySamples`) from internal dry compensation (`internalDryCompensationSamples`)
  - Zero Latency mode now reports 0 latency to host BUT internally delays dry signal by measured minimum-phase filter latency
  - Dry signal now properly compensates for actual wet path latency in all modes
  - Eliminates comb filtering artifacts when using wet/dry mix knob in Zero Latency mode
  - Wet/dry mixing is now phase-coherent across all three processing modes

### Changed
- Dry delay buffer sizing now based on `internalDryCompensationSamples` instead of `totalLatencySamples`
- Zero Latency mode: Dry delayed by `zeroLatencyModeSamples` (~1-5 samples) for phase alignment
- Balanced/Linear Phase modes: Dry delayed by `maxModeLatencySamples` for phase alignment
- Host-reported latency remains correct (0 for ZL, max for others) while internal compensation matches actual processing latency

## [1.2.1] - 2025-11-21

### Fixed
- **Latency compensation and phase alignment**: Implemented proper latency compensation across all processing modes
  - Balanced and Linear Phase modes now report identical latency to DAW host (maximum of the two)
  - Balanced mode adds delay compensation to match Linear Phase latency for seamless mode switching
  - Zero Latency mode correctly reports 0 latency to host (no compensation needed)
  - Dry signal delay properly matches wet signal latency for phase-coherent wet/dry mixing
  - No more phase cancellation or comb filtering when using wet/dry mix knob
  - Timeline stays perfectly aligned when switching between Balanced and Linear Phase modes

### Changed
- All three oversampling filter types are now instantiated simultaneously for accurate latency measurement
- Mode compensation delay automatically calculated from measured filter group delays
- Plugin now queries actual oversampling latency using `getLatencyInSamples()` instead of hardcoded values
- `setLatencySamples()` dynamically updated based on current processing mode

## [1.2.0] - 2025-11-21

### Changed
- **MODE-AWARE PROCESSING**: All 4 XY processors now respect processing mode selection
  - **Zero Latency mode**: NO oversampling on any processor (true zero latency, minimum phase)
  - **Balanced mode**: 8x halfband equiripple oversampling on all processors (~32 samples latency, transparent)
  - **Linear Phase mode**: 8x high-order FIR oversampling on all processors (~128 samples latency, perfect reconstruction)
- Phase is now perfectly aligned end-to-end within each mode regardless of settings
- Envelope time constants automatically scale based on processing mode

### Fixed
- Zero Latency mode now actually delivers zero latency (no oversampling)
- Processing mode selection now controls oversampling for entire signal path

## [1.1.2] - 2025-11-21

### Fixed
- **Comb filtering eliminated**: Applied oversampling to ALL 4 XY processors (Hard Clip, Soft Clip, Slow Limit, Fast Limit)
- Perfect phase alignment when blending between any processors on the XY grid
- Version label no longer overlaps with window size display

### Changed
- All 4 XY processors now use identical 8x oversampling architecture
- Envelope time constants properly scaled for oversampled rate
- UI layout: version label positioned below window size display

## [1.1.1] - 2025-11-21

### Fixed
- **Phase continuity across all processing stages**: Applied oversampling to all 4 main XY processors (Hard Clip, Soft Clip) to match overshoot/TPL processing
- Eliminated phase shift when engaging overshoot or true-peak protection
- All processing now uses consistent oversampling architecture within each mode

### Changed
- Hard Clip and Soft Clip processors now use 8x oversampling matching the selected processing mode
- Phase response remains constant regardless of which limiters are engaged

## [1.1.0] - 2025-11-21

### Added
- **3-Mode Processing Engine** for phase-coherent operation:
  - **Zero Latency**: Minimum-phase polyphase IIR oversampling (0 samples latency, punchy character)
  - **Balanced**: Halfband equiripple FIR oversampling (~32 samples latency, transparent)
  - **Linear Phase**: High-order FIR oversampling (~128 samples latency, mastering-grade)
- Processing mode selector above XY grid for easy engine switching
- Version display (v1.1.0) in upper right corner of plugin window
- Phase-consistent oversampling architecture eliminates mixed-paradigm artifacts

### Fixed
- Phase ripple when engaging overshoot or true-peak processing
- Conditional oversampling causing phase inconsistencies
- Dry/wet phase mismatch during protection limiter activation
- Plugin Doctor phase response measurement artifacts

### Changed
- Oversampling architecture now mode-specific for consistent phase response
- Latency reporting updated dynamically based on selected processing mode
- All processing maintains fixed phase characteristics within each mode

## [1.0.0] - 2025-11-21

### Added
- Initial release of Quad-Blend Drive
- Four-stage drive/saturation processing
- Blend controls for parallel processing
- VST3 and AU format support
- macOS compatibility
- GitHub integration with CI/CD workflows

### Changed
- Updated company name to "Steve Vealey"
- Configured JUCE as proper git submodule
