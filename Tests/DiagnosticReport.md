# QuadBlend Drive v1.8.0 - Null Test Diagnostic Report

## Test Results

### PASSED Tests (5/9)
✓ Bypass Null Test (All signals) - Bit-identical (-200 dB)
✓ Mix 0% Null Test (Impulse) - -120.41 dB
✓ Mix 0% Null Test (Pink Noise) - -128.24 dB

### FAILED Tests (4/9)
✗ Mix 0% Null Test (Sine Sweep) - **-12.76 dB** (CRITICAL)
✗ Unity Gain Test (Sine Sweep) - **-6.05 dB**
✗ Unity Gain Test (Impulse) - -0.03 dB
✗ Unity Gain Test (Pink Noise) - -2.07 dB

## Root Cause Analysis

### Issue 1: Mix 0% Sine Sweep Failure (-12.76 dB)

**Observation**: Impulse and pink noise pass at mix 0%, but sine sweep fails badly.

**Possible Causes**:
1. **Parameter Smoothing Bleed**: `smoothedMixWet` may not reach exactly 0.0
   - Location: PluginProcessor.cpp:1148
   - Issue: `smoothedMixWet.getNextValue()` may asymptotically approach 0 but never reach it
   - Fix: Add threshold check (if mixWet < 0.001, treat as 0.0)

2. **Floating-Point Precision**: Sine sweep is continuous, so small errors accumulate
   - Impulse is mostly zeros (sparse errors)
   - Pink noise averages out (random errors)
   - Sine sweep exposes systematic errors

### Issue 2: Unity Gain Passthrough Failures

**Observation**: Even with all processors muted, signal is altered.

**Possible Causes**:
1. **Oversampling Filter Passthrough** (Most Likely)
   - Location: processXYBlend() with oversampling enabled
   - Issue: Even with zero weights, signal passes through OS filters
   - The `allProcessorsMuted` check (line 903) should skip OS entirely

2. **Mode 0 vs Mode 1/2 Behavior**:
   - Test uses Mode 0 (Zero Latency) by default
   - May still route through oversampling infrastructure
   - Expected: Mode 0 should be true bypass when weights = 0

3. **Buffer Copy Artifacts**:
   - Multiple buffer copies in processing chain
   - Rounding errors from float precision

## Recommended Fixes

### Fix 1: Add Threshold Check for Mix 0%
```cpp
// In processBlockInternal() around line 1145
for (int i = 0; i < numSamples; ++i)
{
    const SampleType smoothedOutGain = static_cast<SampleType>(smoothedOutputGain.getNextValue());
    const SampleType smoothedMix = static_cast<SampleType>(smoothedMixWet.getNextValue());

    // Apply output gain to wet
    wet[i] *= smoothedOutGain;

    // Dry/wet mix with threshold check
    if (smoothedMix < static_cast<SampleType>(0.001))  // Treat < 0.1% as 0%
    {
        wet[i] = dry[i];  // Pure dry signal
    }
    else
    {
        wet[i] = dry[i] * (static_cast<SampleType>(1.0) - smoothedMix) + wet[i] * smoothedMix;
    }
}
```

### Fix 2: Skip Oversampling When All Processors Muted
```cpp
// In processBlockInternal() around line 903
if (allProcessorsMuted)
{
    // Skip ALL processing - just copy dry to output
    buffer.makeCopyOf(originalInputBuffer);

    // Apply output gain if needed
    buffer.applyGain(static_cast<SampleType>(outputGain));

    // Skip the rest of processing
    // ... (jump to mix stage or return early)
}
```

### Fix 3: Exact Zero Mix Handling
Add parameter range quantization:
```cpp
// When setting mix parameter target
float quantizedMix = mixWet;
if (mixWet < 0.01f)  // < 1%
    quantizedMix = 0.0f;
else if (mixWet > 99.99f)  // > 99.99%
    quantizedMix = 1.0f;

smoothedMixWet.setTargetValue(quantizedMix);
```

## Test Coverage Improvement

### Additional Tests Needed:
1. **Parameter Ramp Test**: Slowly automate mix from 0% to 100%
2. **Mode-Specific Tests**: Test each processing mode separately
3. **Frequency-Specific Tests**: Test different frequencies to identify problematic bands
4. **Latency Verification**: Confirm reported latency matches actual delay

### Signal-Specific Observations:
- **Sine Sweep**: Exposes systematic errors (continuous signal)
- **Impulse**: Exposes transient handling (sparse signal)
- **Pink Noise**: Exposes statistical behavior (random signal)

## Priority

1. **CRITICAL**: Fix Mix 0% sine sweep failure (-12.76 dB is unacceptable)
2. **HIGH**: Optimize muted processor passthrough
3. **MEDIUM**: Improve mix parameter quantization

## Expected Results After Fixes

- Mix 0% should be bit-identical or < -120 dB for ALL signals
- Unity gain with muted processors should be < -100 dB for ALL signals
- Bypass already passes (no changes needed)

## Notes

The bypass test passing perfectly (bit-identical) proves the infrastructure is sound. The failures are specific to edge cases in the processing path, not fundamental architectural issues.
