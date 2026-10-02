# Audio32 Driver for macOS

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/build-CMake-informational)
![macOS](https://img.shields.io/badge/platform-macOS-lightgrey)
![Apple Silicon](https://img.shields.io/badge/Apple%20Silicon-ready-lightgrey)

Audio32 is a C++20 macOS audio library designed around a 32-bit floating-point pipeline, low-latency playback experiments, DSP processing, and CoreAudio integration.

The project is under active development. The README separates implemented library surfaces from planned production features to avoid implying that every professional-audio workflow is complete.

## Design Goals

- Real-time-safe audio callback design
- Float32-first processing pipeline
- Minimal external dependencies
- Modern C++20 API surface
- Backend abstraction for future platform support
- Lock-free single-producer / single-consumer buffer movement
- SIMD optimization for hot DSP paths

## Current Features

- 32-bit floating-point audio pipeline
- Configurable internal audio processing buffer
- Low-latency-oriented architecture
- Multi-channel support
- CoreAudio `AudioQueue` output backend
- Thread-safe buffer primitives
- SIMD gain/limiter hot path with scalar fallback
- Modular architecture
- CMake build, example program, and tests

## Experimental Surfaces

- MIDI control-change mapping for DSP parameters
- AudioUnit packaging metadata
- AVFoundation, Metal, Bluetooth, and spatial audio capability boundaries
- In-memory rendered-output recording and loopback capture

## Supported Platforms

Current:

- macOS 14+
- Apple Silicon and Intel Macs supported by the active CMake toolchain
- Linux ALSA and Windows WASAPI implementations (device validation pending)

Future:

- JACK-compatible workflows

## Architecture

```text
Application
      |
      v
Audio32 API
      |
      v
Audio Engine
      |
      +--------------+
      |              |
      v              v
Mixer          DSP Engine
      |              |
      +------+-------+
             v
      Audio Buffer
         (1 MB)
             v
     CoreAudio Driver
             v
       Audio Hardware
```

## Internal Buffer

The driver uses a configurable internal processing buffer.

Default configuration:

| Parameter | Value |
|-----------|-------|
| Buffer Size | 1 MB |
| Sample Format | Float32 |
| Channels | 1+ interleaved channels |
| Sample Rate | Configurable |

## Audio Formats

Current support:

- Interleaved `Float32` samples for the main driver API
- PCM 16-bit, 24-bit, and 32-bit conversion to `Float32`
- Float64 conversion to `Float32`
- LPCM WAV, AIFF, and CAF decoding to interleaved `Float32`
- MP3 and FLAC decoding via pinned miniaudio

Planned:

- Native non-`Float32` processing paths
- AAC and Ogg decoding
- More complete metadata and channel-layout handling

## DSP Features

Current:

- Built-in Neutral, Voice, Room, and Echo presets with versioned effect-chain serialization
- Gain control
- Limiter
- Stereo mixing
- Channel routing
- FFT magnitude analysis
- Three-band biquad EQ with low/high shelves and a parametric mid band
- Stereo-linked soft-knee compressor with attack and release
- Damped comb/all-pass reverb with independent channel tails
- Fractional delay with feedback and smoothed delay-time changes

Planned:

- Additional filter types and compressor detector modes
- Additional effect presets
- Broader spectrum analysis tools

See [DSP configuration and processing contract](docs/dsp.md) for parameter ranges, lifecycle, and validation limits.

## Thread Model

```text
Main Thread
      |
      v
Control Thread
      |
      v
Real-Time Audio Thread
      |
      v
CoreAudio Callback
```

The real-time thread avoids:

- Dynamic memory allocation
- Locks
- Exceptions
- Blocking I/O

## Memory Management

```text
+-------------------------+
| Audio Buffer Pool       |
+-------------------------+

1 MB Circular Buffer

Read Pointer
Write Pointer

Lock-Free
```

The circular buffer is intended for single-producer / single-consumer audio flow.

## Thread Safety

| Component | Status |
|-----------|--------|
| `AudioBuffer` | Lock-free SPSC buffer for audio sample movement |
| `AudioDriver` | Safe for basic control and write flow; avoid mutating DSP settings from the render callback |
| `DspEngine` | Real-time-friendly during `process()` after effect state has been configured |
| `CoreAudioBackend` | Owns backend lifecycle state and render callback handoff |

## Performance Targets

- Target latency: below 5 ms
- Target behavior: no audio dropouts under normal load
- Lock-free audio-buffer movement
- Real-time-friendly render callback
- High throughput on Apple Silicon

Repeatable DSP processing benchmarks, raw CSV results, and machine/compiler metadata
are published in [the benchmark report](docs/benchmarks.md). The benchmark measures
processing duration against the audio buffer period; it does not measure end-to-end
hardware playback latency.

At 48 kHz stereo with all effects enabled, the recorded Apple M4 run measured
6.625 µs p99 for 128 frames (2,666.67 µs buffer period). Results depend on hardware,
scheduler load, compiler, and build configuration.

## Build

Requirements:

- macOS 14+
- Clang
- CMake 3.25+
- Xcode Command Line Tools

## macOS Quick Start

Step 1: Install requirements:

```bash
xcode-select --install
brew install cmake
```

Step 2: Build:

```bash
cmake -S . -B build
cmake --build build -j8
```

If an existing build cache refers to a former project location, regenerate it:

```bash
cmake --fresh -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
```

Release build:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j8
```

Debug build:

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j8
```

Step 3: Run:

```bash
ctest --test-dir build --output-on-failure
./build/audio32_example
```

Step 4: Install on macOS:

```bash
cmake --install build --prefix /usr/local
```

If `/usr/local` requires administrator access:

```bash
sudo cmake --install build --prefix /usr/local
```

To install without administrator access:

```bash
cmake --install build --prefix "$HOME/.local"
```

## Example

```cpp
#include "audio_driver.hpp"

#include <chrono>
#include <cmath>
#include <numbers>
#include <thread>
#include <vector>

audio32::AudioDriver driver;

driver.setSampleRate(48000.0);
driver.setChannels(2);
driver.initialize();

std::vector<float> samples(48000 * 2);
for (std::size_t frame = 0; frame < 48000; ++frame) {
    const auto t = static_cast<float>(frame) / 48000.0F;
    const auto sample = std::sin(2.0F * std::numbers::pi_v<float> * 440.0F * t);
    samples[frame * 2] = sample;
    samples[frame * 2 + 1] = sample;
}

driver.write(samples);
driver.start();
std::this_thread::sleep_for(std::chrono::seconds(1));
driver.stop();
```

## API Overview

Primary `AudioDriver` methods:

- `initialize()`
- `start()`
- `stop()`
- `setSampleRate()`
- `setChannels()`
- `setOutputDeviceUid()`
- `write()`
- `dsp()`
- `mixer()`
- `setRecordingEnabled()`
- `takeRecordedSamples()`
- `setLoopbackCaptureEnabled()`
- `takeLoopbackSamples()`

Supporting modules:

- `audio_io`: PCM conversion and LPCM container decoding
- `DspEngine`: gain, limiter, analysis, and stateful effects
- `Mixer`: channel routing and mono/stereo expansion
- `MidiMapper`: MIDI CC mapping for DSP parameters
- `CoreAudioBackend`: macOS playback backend boundary

## Project Structure

```text
Audio32/
+-- include/   Public headers
+-- src/       Library implementation
+-- examples/  Basic playback example
+-- tests/     CTest-based test executable
+-- docs/      Design notes
`-- build/     Generated local build output
```

## Backend Status

Current:

- CoreAudio `AudioQueue` output on macOS
- Native AudioUnit, ALSA, and WASAPI playback/capture

Planned or research:

- PulseAudio and JACK integration
- Wider device and host compatibility validation

## Current Limitations

- Native playback/capture use AudioUnit on macOS, ALSA on Linux, and WASAPI on Windows; hardware validation is currently macOS only.
- The production macOS output path uses `AudioQueue`.
- The main driver API processes interleaved `Float32` samples.
- The optional JUCE AudioUnit plugin builds and passes a host test; DAW compatibility, signing, and notarization remain.
- `AudioDriver` recording/loopback remain rendered-output capture; `AudioInputCapture` provides physical device input.
- Metal, Bluetooth, and spatial audio support are capability/reporting boundaries, not full user-facing workflows.

## Roadmap

Checked items represent implemented library features or published repository artifacts.
Hardware validation and broader integrations remain separate milestones.

### Phase 1: Core Library

- [x] Public `AudioDriver` facade
- [x] Fixed-size circular audio buffer
- [x] Mixer with channel routing
- [x] DSP pipeline with gain, limiter, and analysis hooks
- [x] CoreAudio backend boundary
- [x] CMake build, example app, and tests

### Phase 2: macOS Playback Backend

- [x] Wire the CoreAudio render callback to an AudioQueue output path
- [x] Add output device UID selection and Float32 stream configuration
- [x] Handle stop/start recovery and backend error reporting
- [x] Add manual validation through the playback example

### Phase 3: Format and I/O Support

- [x] Add integer PCM to `Float32` conversion
- [x] Add WAV, AIFF, and CAF LPCM container decoding
- [x] Add rendered-output recording capture
- [x] Add output loopback capture

### Phase 4: Pro Audio Features

- [x] Add DSP framework for effects
- [x] Add AudioUnit packaging metadata
- [x] Add MIDI control mapping API
- [x] Add AVFoundation integration boundary
- [x] Production-grade equalizer DSP implementation (see [DSP contract](docs/dsp.md))
- [x] Production-grade compressor DSP implementation (see [DSP contract](docs/dsp.md))
- [x] Production-grade reverb DSP implementation (see [DSP contract](docs/dsp.md))
- [x] Production-grade delay DSP implementation (see [DSP contract](docs/dsp.md))

### Phase 5: Performance and Platform Polish

- [x] Add ARM64 and Apple Silicon DSP capability detection
- [x] Add standalone SIMD gain/limiter helper
- [x] Add Metal visualization data preparation
- [x] Add Bluetooth and spatial audio capability reporting boundaries
- [x] Publish [repeatable DSP processing latency benchmarks and baseline results](docs/benchmarks.md)
- [x] Add ASan/UBSan and ThreadSanitizer workflows
- [x] Add concurrent SPSC buffer and randomized DSP stress tests
- [x] Add a manual benchmark workflow with CSV and machine metadata artifacts

### Phase 6: Deployment Validation and Expanded Audio Support

See [implementation details and deployment evidence](docs/phase6.md).

- [ ] Run sanitizer and stress workflows on hosted macOS and Linux runners — [attempt blocked by GitHub account billing](https://github.com/BrunoFerreira995/audiodrive/actions/runs/37054644906)
- [ ] Publish hardware playback and physical round-trip latency measurements
- [ ] Validate effect sound quality through listening tests
- [x] Add effect presets and versioned effect-chain serialization
- [x] Implement a native CoreAudio AudioUnit render backend
- [x] Add hardware input capture API
- [x] Build an AudioUnit effect plugin with automation, editor, and host state
- [x] Add MP3 and FLAC compressed audio decoding
- [x] Add ALSA and WASAPI native backend implementations
- [ ] Validate native devices on Linux and Windows
- [x] Record sustained hardware playback under CPU load with buffer starvation and callback timing counters
- [ ] Measure device-level underruns under sustained load
- [ ] Validate plugin in DAWs and with auval; sign/notarize release artifacts

## Research

- Dolby Atmos compatibility
- Neural noise reduction
- Voice enhancement
- Spatial rendering
- Live effects workflow
- Virtual mixer
- Plugin SDK

## License

MIT License
