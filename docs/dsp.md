# DSP configuration and processing contract

`DspEngine` processes interleaved Float32 frames in this order: gain, EQ,
compressor, reverb, delay, output ceiling. Construction and `setFormat()` prepare
all storage; `process()` performs no allocation, locking, or I/O. Setters, getters,
reset, and processing require external synchronization. Configure between render
calls; simultaneous control/render access is unsupported.

Pass complete frames. A trailing incomplete frame is left untouched. Processing
is independent of block size. `reset()` clears filter/envelope state and effect
tails and snaps smoothed parameters to their targets. Format changes reset all
state. Effect setters preserve existing tails. Dry effects continue updating their
state so that enabling wet output can reveal an existing tail.

## Equalizer

Three cascaded biquads: a low shelf, a peaking mid band, and a high shelf. Gains
are linear amplitude multipliers (1 is unity), bounded to 0.001–16. Defaults are
150 Hz, 1 kHz, and 6 kHz; frequencies are limited to 20 Hz–45% of sample rate.
Mid Q is 0.1–10. Normalized coefficients approach their targets with a 10 ms
one-pole smoothing time. Unity gains produce a unity response.

## Compressor

The maximum absolute channel amplitude drives one linked peak envelope,
preserving channel balance. Attack (0.01–1000 ms) and release (1–10000 ms) are
one-pole time constants. Threshold is linear amplitude, bounded to 0.000001–1;
ratio is 1–100; knee is 0–24 dB. Gain reduction follows a decibel-domain ratio
with a quadratic soft knee. Makeup gain is linear, 0–64. There is no lookahead;
attack transients can pass through and are bounded by the final hard ceiling.
Compressor parameter changes are applied at the next process call.

## Reverb

Each channel has four parallel damped feedback combs followed by two all-pass
stages. Delay lengths scale with sample rate and differ across channels. Channels
have independent state (no stereo crossfeed). Mix is 0–1, feedback is 0–0.95,
and damping is 0–0.99. Mix and feedback use 10 ms smoothing. This is an
algorithmic room tail; feedback is not an RT60 parameter.

## Delay

A preallocated two-second ring supports independent channel echoes. Delay time
is specified in frames and bounded to two seconds; zero bypasses the wet output.
Linear interpolation supports fractional read positions during delay-time changes.
Mix (0–1), feedback (0–0.95), and delay time use 10 ms smoothing. Time modulation
can audibly change pitch. Mix is linear dry/wet. No tempo sync or ping-pong mode.

## Bounds and validation

Sample rates are bounded to 8–384 kHz, channels to 1–64. Nonfinite parameters use
safe defaults; nonfinite input samples are replaced with silence. Tiny recursive
states are flushed to zero. The limiter ceiling and global gain remain immediate
controls. Format changes clamp delay time to the new capacity.

The automated DSP suite checks shelf/peak frequency response, unity response,
compressor steady-state ratio and channel linking, attack behavior, echo timing
and feedback, channel isolation, tail preservation/reset, reverb decay, finite
output, and block partition invariance. These are implemented DSP building
blocks; hardware callback benchmarks and listening validation are still needed
before certifying a particular deployment's production latency and sound quality.
