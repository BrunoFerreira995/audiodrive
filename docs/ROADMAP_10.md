# Audio32 — Roadmap to 10/10

## P0 — Real-time Safety & CI (Critical)
- [x] Document callback contracts (non-allocating, lock-free) 
- [ ] Add real-time audit: enforce no malloc/new/lock in render paths (RT-safety linter/check)
- [ ] Harden native_audio.cpp: null buffer checks, overflow protection, error propagation
- [ ] Fix device ID stability on Linux/Windows (use stable identifiers + re-enumeration)
- [ ] Unblock and fix hosted sanitizers/TSAN on all runners
- [ ] Add strict CI gates (fail on sanitizer warnings)

## P0 — Latency & Hardware Validation
- [ ] Measure physical round-trip latency with loopback (published CSV + analysis)
- [ ] Measure device underruns/xruns under sustained load
- [ ] Add callback jitter measurement to hardware benchmark
- [ ] Publish end-to-end latency methodology

## P1 — DSP Quality
- [ ] Replace O(N²) FFT with FFTW or simple Cooley-Tukey (O(N log N))
- [ ] Implement true-peak aware limiter with lookahead (or brickwall limiter)
- [ ] Add compressor detector modes (RMS/peak)
- [ ] Add more filter types (high-pass/low-pass) 
- [ ] Improve parameter smoothing (per-sample or block-based ramps)

## P1 — Plugin Distribution Readiness
- [x] Factory presets, units, custom editor, bypass, MIDI CC learn, .chain load/save
- [x] VST3 support alongside AU
- [x] auval scripts/workflows
- [x] Code signing/notarization scripts/workflows
- [ ] Test in Logic Pro, Reaper, GarageBand
- [ ] Generate signed/notarized release artifacts
- [ ] Add VST3 on Linux/Windows validation

## P2 — Testing & Observability
- [x] Stress tests, sanitizers, benchmarks
- [ ] Property-based tests for DSP
- [ ] Fuzzing for file decoders (WAV/AIFF/MP3/FLAC)
- [ ] Real-time metrics export (callback timing histogram)
- [ ] Golden audio tests for effects

## P2 — Polish
- [ ] Custom plugin UI with VU meters/GR meter
- [ ] Preset browser with thumbnails
- [ ] MIDI learn UI improvements
- [ ] Documentation: architecture deep dive, RT-safety guide
- [ ] Performance regression detection in CI

**Target Score:** 10/10 (stability + validation + distribution)
