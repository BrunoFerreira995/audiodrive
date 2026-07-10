# Audio32 Driver for macOS

High-performance 32-bit audio driver for macOS designed for low-latency playback, high-fidelity digital audio processing, and professional audio applications.

## Features

- 32-bit floating-point audio pipeline
- Up to 1 MB internal audio processing buffer
- Low latency architecture
- Multi-channel support
- High dynamic range
- CoreAudio integration
- Real-time processing
- Thread-safe audio engine
- SIMD-ready DSP pipeline
- Modular architecture

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
| Channels | Mono / Stereo / Surround |
| Sample Rate | 44.1 kHz - 384 kHz |

## Audio Formats

Supported pipeline formats:

- PCM 16-bit
- PCM 24-bit
- PCM 32-bit
- Float32
- Float64
- WAV
- AIFF
- CAF

The library API accepts interleaved `Float32` samples. The `audio_io` module provides integer PCM conversion and LPCM WAV, AIFF, and CAF decoding into the same interleaved `Float32` representation.

## DSP Features

- Volume Control
- Stereo Mixing
- Channel Routing
- Limiter
- FFT Analysis

Planned:

- Equalizer
- Compressor
- Reverb
- Delay

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

## Performance Goals

- Latency below 5 ms
- Zero audio dropouts
- Lock-free processing
- Real-time scheduling
- High throughput

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

audio32::AudioDriver driver;

driver.initialize();
driver.setSampleRate(48000);
driver.setChannels(2);
driver.start();
```

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

- [x] Add AudioUnit plugin packaging metadata
- [x] Add MIDI control mapping
- [x] Add equalizer, compressor, reverb, and delay modules
- [x] Add AVFoundation integration boundary

### Phase 5: Performance and Platform Polish

- [ ] Add ARM64 and Apple Silicon DSP optimizations
- [ ] Add SIMD implementations for hot DSP paths
- [ ] Add Metal-based visualization
- [ ] Explore Bluetooth and spatial audio support

## Future Features

- Dolby Atmos
- AI Noise Reduction
- Voice Enhancement
- Live Effects
- Virtual Mixer
- Plugin SDK
- Audio Recording
- Loopback Capture

## License

MIT License
