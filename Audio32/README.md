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

- Basic equalizer, compressor, reverb, and delay modules
- MIDI control-change mapping for DSP parameters
- AudioUnit packaging metadata
- AVFoundation, Metal, Bluetooth, and spatial audio capability boundaries
- In-memory rendered-output recording and loopback capture

## Supported Platforms

Current:

- macOS 14+
- Apple Silicon and Intel Macs supported by the active CMake toolchain

Future:

- Linux audio backends
- Windows audio backends
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

Planned:

- Native non-`Float32` processing paths
- Broader compressed format support through platform media APIs
- More complete metadata and channel-layout handling

## DSP Features

Current:

- Gain control
- Limiter
- Stereo mixing
- Channel routing
- FFT magnitude analysis
- Basic equalizer, compressor, reverb, and delay modules

Planned:

- Production-grade filter design
- Parameter smoothing
- Presets and effect-chain serialization
- Broader spectrum analysis tools

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

Benchmarks are not published yet. A future benchmark suite should report buffer size, sample rate, channel count, build type, hardware, and measured callback latency.

Example benchmark table format:

| Buffer | Sample Rate | Measured Latency |
|--------|-------------|------------------|
| 128 frames | 48 kHz | TBD |
| 256 frames | 48 kHz | TBD |
| 512 frames | 48 kHz | TBD |

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
- `DspEngine`: gain, limiter, analysis, and basic effects
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

Planned or research:

- CoreAudio `AudioUnit` render path
- Hardware input device capture
- ALSA, WASAPI, PulseAudio, and JACK backends

## Current Limitations

- Playback is the only hardware audio path currently implemented.
- The production macOS output path uses `AudioQueue`.
- The main driver API processes interleaved `Float32` samples.
- AudioUnit support is metadata/boundary work, not a complete plugin target.
- Recording and loopback capture are in-memory rendered-output capture paths, not hardware input capture.
- Metal, Bluetooth, and spatial audio support are capability/reporting boundaries, not full user-facing workflows.

## Roadmap

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
- [ ] Production-grade equalizer
- [ ] Production-grade compressor
- [ ] Production-grade reverb
- [ ] Production-grade delay

### Phase 5: Performance and Platform Polish

- [x] Add ARM64 and Apple Silicon DSP capability detection
- [x] Add SIMD implementation for the gain/limiter hot path
- [x] Add Metal visualization data preparation
- [x] Add Bluetooth and spatial audio capability reporting boundaries
- [ ] Publish repeatable latency benchmarks
- [ ] Add sanitizer and stress-test workflows

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
