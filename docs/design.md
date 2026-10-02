# Audio32 Design Notes

Audio32 is structured as a small C++20 library with four main components:

- `AudioDriver`: public facade for configuration, lifecycle, and sample submission.
- `AudioBuffer`: fixed-size circular buffer for interleaved `float` samples.
- `Mixer`: channel mapping and mono/stereo expansion utilities.
- `DspEngine`: gain, limiting, and analysis hooks.
- `CoreAudioBackend`: macOS output backend boundary.
- `audio_io`: integer PCM conversion, LPCM WAV/AIFF/CAF and MP3/FLAC decoding.
- `NativeAudioBackend`: AudioUnit/ALSA/WASAPI playback and capture through pinned miniaudio.
- `AudioInputCapture`: physical-input SPSC capture with overflow reporting.
- `effect_chain`: presets and versioned settings serialization.
- `MidiMapper`: MIDI control-change mapping for DSP parameters.
- `AudioUnitPluginDescriptor`: AudioUnit packaging metadata.
- `AVFoundationIntegration`: macOS AVFoundation capability boundary.
- `simd`: scalar and ARM NEON helpers for hot DSP paths.
- `PlatformCapabilities`: macOS, ARM64, Apple Silicon, and SIMD capability reporting.
- `metal_visualization`: spectrum-bar data preparation for Metal renderers.
- `WirelessAudioCapabilities`: Bluetooth and spatial audio capability reporting.

The current macOS backend uses `AudioQueue` output with a `Float32` stream description, fixed-size output buffers, optional output device UID selection, and backend error reporting. The render callback reads from the preallocated circular buffer and applies in-place DSP before each buffer is re-enqueued.

`AudioDriver` can capture rendered output into bounded in-memory recording and loopback buffers. Capture is intended for diagnostics, export, and internal routing; it does not yet open a hardware input device.

The DSP engine includes gain, limiting, three-band equalizer shaping, compressor, reverb, and delay modules. Stateful DSP storage is allocated at construction and format changes, not during normal sample processing or effect parameter updates.

The standalone gain/limiter SIMD helper selects ARM NEON on supported targets and falls back to scalar processing elsewhere; the stateful DSP engine uses its frame processing loop. Visualization support prepares normalized spectrum bar data that can be consumed by a Metal renderer without coupling the audio engine to a UI surface.

Real-time code paths should avoid allocation, blocking I/O, locks, and exceptions.
