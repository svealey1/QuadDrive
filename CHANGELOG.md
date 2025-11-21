# Changelog

All notable changes to Quad-Blend Drive will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
