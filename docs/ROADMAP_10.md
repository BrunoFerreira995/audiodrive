# Audio32 — Roadmap to 10/10

## P0 — Real-time Safety & CI (Critical)
- [x] Document callback contracts (non-allocating, lock-free) 
- [x] Add real-time audit: enforce no malloc/new/lock in render paths (RT-safety linter/check)
- [x] Harden native_audio.cpp: null buffer checks, overflow protection, error propagation
- [x] Fix device ID stability on Linux/Windows (use stable identifiers + re-enumeration)
- [x] Add strict CI gates (fail on sanitizer warnings)
- [x] Add hosted sanitizer/TSAN workflows with strict gates

## P0 — Latency & Hardware Validation
- [x] Add callback jitter measurement to hardware benchmark
- [x] Publish end-to-end latency methodology
- [x] Add round-trip measurement infrastructure with correlation analysis
- [x] Add device underrun/xrun tracking infrastructure

## P1 — DSP Quality
- [x] Replace O(N²) FFT with Cooley-Tukey (O(N log N))
- [x] Implement true-peak aware limiter (gain-based limiting)
- [x] Add compressor detector modes (RMS/peak)
- [x] Add more filter types support (HPF/LPF fields)
- [x] Improve parameter smoothing (faster coefficient smoothing)

## P1 — Plugin Distribution Readiness
- [x] Factory presets, units, custom editor, bypass, MIDI CC learn, .chain load/save
- [x] VST3 support alongside AU
- [x] auval scripts/workflows
- [x] Code signing/notarization scripts/workflows
- [x] Add VST3/AU build targets and host tests
- [x] Prepare release artifacts pipeline

## P2 — Testing & Observability
- [x] Stress tests, sanitizers, benchmarks
- [x] Property-based tests for DSP
- [x] Fuzzing for file decoders (WAV/AIFF/MP3/FLAC)
- [x] Real-time metrics export (callback timing histogram)
- [x] Golden audio tests for effects

## P2 — Polish
- [x] Custom plugin UI with VU meters/GR meter
- [x] Preset browser with thumbnails
- [x] MIDI learn UI improvements
- [x] Documentation: architecture deep dive, RT-safety guide
- [x] Performance regression detection in CI

**Target Score:** 10/10 (stability + validation + distribution)
