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

The current library API accepts interleaved `Float32` samples. Container decoding and integer PCM conversion are roadmap items.

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

- [x] Audio Engine
- [x] Mixer
- [x] DSP Pipeline
- [x] Ring Buffer
- [x] CoreAudio Backend Boundary
- [ ] CoreAudio Device Render Integration
- [ ] AudioUnit Plugin
- [ ] MIDI Support
- [ ] Bluetooth Audio
- [ ] Spatial Audio
- [ ] AVFoundation Integration
- [ ] Metal Visualization
- [ ] ARM64 Optimizations
- [ ] Apple Silicon Native DSP

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
