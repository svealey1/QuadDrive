# QuadBlend Drive - Null Test Suite

## Overview

Automated null tests to verify bypass transparency, mix correctness, and unity gain behavior for QuadBlend Drive v1.8.0.

## Tests Included

### Test 1: Bypass Null Test
Verifies that when bypass is enabled, the output is bit-identical to the input (no processing artifacts).

**Expected Result**: Bit-identical or < -120 dB difference

### Test 2: Mix 0% Null Test
Verifies that when mix is set to 0% (fully dry), the output matches the pristine dry signal regardless of processing settings.

**Expected Result**: Bit-identical or < -120 dB difference

### Test 3: Unity Gain Test
Verifies that with all processors muted and unity gain, the signal passes through transparently.

**Expected Result**: < -100 dB difference (allows for oversampling filter passthrough)

## Test Signals

Each test runs on three signal types:
- **Sine Sweep**: 20 Hz to 20 kHz sweep (tests frequency response)
- **Impulse**: Single impulse (tests time-domain accuracy)
- **Pink Noise**: Broadband noise (tests statistical behavior)

## Building the Tests

### Step 1: Enable test build option
```bash
cd /Users/stevevealey/Projects/QuadDrive
cmake -B build -DBUILD_TESTS=ON
```

### Step 2: Build the test executable
```bash
cmake --build build --config Release --target NullTest
```

## Running the Tests

```bash
./build/Tests/NullTest
```

## Interpreting Results

### Pass Criteria
- **Bypass Test**: Must be bit-identical or have max difference < -120 dB
- **Mix 0% Test**: Must be bit-identical or have max difference < -120 dB
- **Unity Gain Test**: Max difference < -100 dB (allows for oversampling)

### Expected Output
```
=======================================
QuadBlend Drive v1.8.0 - Null Test Suite
=======================================

=== TEST 1: Bypass Null Test (Sine Sweep) ===
Bit-identical: PASS
Max difference: -200.0 dB
Result: PASS

[... additional tests ...]

=======================================
TEST SUMMARY
=======================================
Total Tests: 9
Passed: 9
Failed: 0
Pass Rate: 100%
=======================================

✓ ALL TESTS PASSED!
```

## Troubleshooting

### Test fails with > -100 dB difference
- Check if latency compensation is properly configured
- Verify oversampling is being reset between tests
- Ensure parameter smoothers are initialized correctly

### Bypass not bit-identical
- Verify bypass path truly bypasses all processing
- Check if any gain/normalization is being applied in bypass mode
- Confirm buffer copy operation is working correctly

### Mix 0% not bit-identical
- Verify dry signal path is pristine (no gains applied)
- Check if dry/wet mixing is mathematically correct
- Ensure dry delay compensation is disabled for Mode 0

## Implementation Details

### Signal Generation
- **Sine Sweep**: Logarithmic sweep from 20 Hz to 20 kHz
- **Impulse**: Unit impulse at sample 100
- **Pink Noise**: Generated using Paul Kellet's pink noise filter

### Difference Measurement
- Computes absolute difference per sample
- Reports maximum difference across all channels and samples
- Converts to dB: `20 * log10(maxDiff)`

### Bit-Identical Check
- Direct float comparison (no tolerance)
- Requires exact binary match for all samples

## Integration with CI/CD

This test suite can be integrated into continuous integration pipelines:

```bash
#!/bin/bash
# CI test script
cmake -B build -DBUILD_TESTS=ON
cmake --build build --config Release --target NullTest
./build/Tests/NullTest
EXIT_CODE=$?
exit $EXIT_CODE
```

## Future Enhancements

- [ ] Add FFT-based frequency response tests
- [ ] Add THD+N measurements
- [ ] Add latency verification tests
- [ ] Add parameter automation tests
- [ ] Export results to JSON for automated analysis
