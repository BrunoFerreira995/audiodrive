# Expanded audio support and deployment evidence

## Implemented library APIs

`NativeAudioBackend` supports interleaved Float32 playback, capture, and duplex
operation. It uses miniaudio 0.11.23 with an explicitly selected native backend:
macOS CoreAudio AudioUnit, Linux ALSA, or Windows WASAPI. A null backend is never
selected. Device errors are reported rather than pretending playback started.
The default `AudioDriver` remains AudioQueue on macOS and uses the native backend
on Linux/Windows. On macOS select `OutputBackend::Native` for AudioUnit output:

```cpp
AudioDriver driver;
driver.setOutputBackend(OutputBackend::Native);
driver.setPeriodFrames(128);
driver.initialize();
```

`NativeAudioBackend::devices()` enumerates input/output IDs and names. macOS IDs
are CoreAudio UIDs; Linux/Windows IDs are enumeration indices scoped to the current
device list, so re-enumerate after hotplug. An empty ID selects the system default.
Configuration, enumeration, start, stop, and destruction require exclusive control
access. The user-supplied callback must avoid allocation, locks, I/O, and exceptions.
Requested periods are hints; hardware may negotiate a different period/rate.

`AudioInputCapture` buffers physical input in an SPSC ring. Configure/start/stop
on the control thread; one consumer calls `read()` and checks `droppedSamples()`.
Overflow drops newly captured samples rather than overwriting unread data. Reads
return sample counts, zero-fill unused output, and use interleaved channels.
Microphone/device access may require OS permission. This API is separate from
`AudioDriver`'s rendered-output recording and software loopback capture.

`decodeAudioFile()` retains LPCM WAV/AIFF/CAF decoding and adds miniaudio MP3 and
FLAC decoding to Float32 at the source sample rate/channel count. Decoding is
non-real-time, loads the source into memory, and caps decoded output at 1 GiB.
AAC and Ogg are not included. Synthetic, repository-owned MP3/FLAC fixtures are
checked for format, length, finite samples, and non-silent output.

`EffectChainSettings` stores gain, ceiling, and all four effect settings. Presets
are Neutral, Voice, Room, and Echo. Echo converts a 250 ms time to frames using
the supplied sample rate. Capture/apply helpers operate on `DspEngine`; applying
can preserve or clear tails. Version 1 serialization uses locale-independent text
and round-trip float precision. Parsing rejects unknown versions, missing fields,
extra data, nonfinite values, and overflowing frame counts. Numeric parameters
are clamped by the engine on application. Serialization stores settings, not
running delay/reverb/filter state. The chain order is fixed.

The optional [AUv2 plugin](../plugins/README.md) provides a host editor,
automatable parameters, and persistent host state. Its build and in-process
render/state test passed locally.

## Actual hardware playback under load

Build and enumerate:

```sh
cmake -S . -B build-hardware -DCMAKE_BUILD_TYPE=Release -DAUDIO32_BUILD_HARDWARE_TOOLS=ON
cmake --build build-hardware --parallel 4
./build-hardware/audio32_hardware_benchmark --list
./build-hardware/audio32_hardware_benchmark --playback 60 4 playback.csv
```

Playback generates a low-amplitude 440 Hz signal, primes a small SPSC ring, feeds
it from a producer thread, and processes the voice preset plus reverb and delay.
Four worker threads continuously execute floating-point sine work. Timing covers
the callback's buffer read and DSP; it excludes backend scheduling and device
I/O. Read shortages count buffer starvations; callback budget overruns compare
work duration to that callback's actual frame count at the configured sample rate.
These counters do not measure device-level ALSA xruns/WASAPI glitches/CoreAudio
hardware underruns or physical playback latency.

[Recorded CSV](../benchmarks/results/2026-10-02-playback-load.csv) and
[machine/source metadata](../benchmarks/results/2026-10-02-playback-load.json): Apple M4,
macOS 27.0.1, AppleClang 21.0.0, Release, TEYUN Q26 system-default USB output,
48 kHz stereo, requested 128-frame period, 60 seconds, four load threads.
22,501 callbacks; **0 buffer starvations**, **0 callback budget overruns**;
p50 **4.375 µs**, p99 **5.875 µs**, maximum **10.709 µs**; backend error 0.
This is one sustained run, not a device-level underrun guarantee.

## Physical round-trip latency: awaiting a connected loopback

Connect a physical output to a compatible line input, set appropriate levels,
and supply device IDs from `--list`:

```sh
./build-hardware/audio32_hardware_benchmark --roundtrip 8 0 roundtrip.csv OUTPUT_ID INPUT_ID
```

The duplex callback emits a deterministic 511-sample bipolar burst each second
and captures the first input channel. Offline normalized cross-correlation scans
up to one second after each burst and accepts only peaks at least 0.7. Accepted
rows report lag frames and milliseconds at 48 kHz. No confident match exits with
an error and provides no latency result. A successful result measures the selected
physical path's output-to-input round trip, including conversions and buffers;
it cannot separate DAC/output latency from ADC/input latency. No physical result
has been published yet: the cable/device setup must be confirmed and measured.

## Listening validation: materials ready, human assessment pending

Generate dry reference, individual effects, and presets from speech/music/drums:

```sh
./build-hardware/audio32_render_effects source.wav listening-output
```

The tool writes eight PCM16 WAV files and corresponding `.chain` settings,
adding four seconds for effect tails. Use a representative source with headroom;
level-match dry/wet comparisons externally. Listen for clicks during transitions,
EQ tonal balance, compressor pumping/transients, reverb decay/metallic coloration,
and delay timing/tails. Include mono/stereo, silence, and transient-heavy material.
Record source, device, headphones/speakers, levels, settings, listener, observations,
and acceptance decisions. No human listening result is claimed by automated tests. Synthetic chirp, tone
bursts, noise transients, and silence have been rendered into the local ignored
`listening-output/` directory as a starting point; speech and music should also be tested.

## Hosted execution

The validation workflow covers macOS ASan/UBSan, macOS TSan, Linux ASan/UBSan,
and optimized macOS/Windows builds, plus ten repeated stress runs. A separate
macOS plugin job builds the AU artifact and runs the host test. Hosted results
were requested by pushing `codex/phase6-audio-validation`.

[Hosted run 37054644906](https://github.com/BrunoFerreira995/audiodrive/actions/runs/37054644906)
failed before any job steps ran. GitHub's check annotation states: “The job was
not started because your account is locked due to a billing issue.” Hosted macOS,
Linux, and Windows validation therefore remains blocked until the account owner
resolves billing and reruns the workflow. Local Release, ASan/UBSan, TSan, and
AudioUnit host tests passed; they do not substitute for hosted results.
