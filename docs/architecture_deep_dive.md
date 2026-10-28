# Architecture Deep Dive

## Pipeline
Application -> Audio32 API -> Engine (Mixer/DSP) -> Buffer (SPSC) -> Backend (CoreAudio/ALSA/WASAPI/AudioUnit) -> Hardware

## Real-time constraints
- Render callback: no allocation, locks, I/O, exceptions
- Lock-free SPSC buffer
- Parameter updates via atomics/smoothing
- Preallocated state

## DSP
Float32 pipeline, biquad EQ, compressor (peak/RMS), reverb, delay, limiter. Cooley-Tukey FFT for analysis.

## Backends
AudioQueue (macOS default), Native (AudioUnit/ALSA/WASAPI), AUv2 plugin via JUCE.
