# Audio32 Design Notes

Audio32 is structured as a small C++20 library with four main components:

- `AudioDriver`: public facade for configuration, lifecycle, and sample submission.
- `AudioBuffer`: fixed-size circular buffer for interleaved `float` samples.
- `Mixer`: channel mapping and mono/stereo expansion utilities.
- `DspEngine`: gain, limiting, and analysis hooks.
- `CoreAudioBackend`: macOS output backend boundary.

The current backend initializes and manages lifecycle state, with the render callback boundary in place. A production CoreAudio implementation should wire this callback into an `AudioQueue`, `AudioUnit`, or `AudioDeviceIOProc` path depending on the deployment target.

Real-time code paths should avoid allocation, blocking I/O, locks, and exceptions. The render method currently reads from the preallocated circular buffer and applies in-place DSP.
