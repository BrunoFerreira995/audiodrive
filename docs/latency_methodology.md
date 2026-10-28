# End-to-end latency methodology

## DSP/internal latency (measured)
- `benchmarks/run.py` measures `DspEngine::process()` wall time vs buffer period. Excludes hardware scheduling.
- `tools/hardware_benchmark` measures callback processing time (read + DSP) under load. Reports p50/p99/max and callback jitter (std dev).

## Physical round-trip latency (measured when available)
- Generate deterministic burst per second on output
- Capture input simultaneously via duplex callback
- Cross-correlate to find peak with correlation >= 0.7
- Report lag frames and ms at sample rate
- Includes DAC + analog + ADC + driver + buffers

## Limitations
- Backend scheduling latency not included in callback timing
- OS power management affects results
- Use same machine/power mode/compiler for comparisons
- No underrun guarantee from timing alone
