# Audio32 AudioUnit plugin

The optional macOS AUv2 effect wraps `DspEngine` through pinned JUCE 8.0.12.
It supports mono/stereo, host automation for all 21 exposed controls, a generic
parameter editor, XML parameter state restoration, and preallocated interleaving
storage. Oversized host blocks are processed in chunks. EQ/compressor/reverb/delay
order is fixed. Delay time is in seconds in the plugin and frames in the library.

```sh
cmake -S . -B build-au -DCMAKE_BUILD_TYPE=Release -DAUDIO32_BUILD_AU_PLUGIN=ON
cmake --build build-au --target Audio32Plugin_AU audio32_au_host_tests --parallel 4
ctest --test-dir build-au -R au_host --output-on-failure
```

CMake downloads JUCE with a verified SHA-256. For an existing checkout, specify
`-DAUDIO32_JUCE_SOURCE_DIR=/absolute/path/to/JUCE-8.0.12`. The resulting component
is under `build-au/plugins/Audio32Plugin_artefacts/Release/AU/`.
Builds do not install or activate it. To use it in a DAW, copy the component into
your user Audio/Plug-Ins/Components directory and rescan in the host.

The in-process host test loads the binary without installing it, registers its
factory, configures stereo Float32 at 48 kHz, renders 20 blocks, and checks class
state save/restore. It passed locally. Full DAW compatibility, `auval`, signing,
and notarization remain release validation tasks.

Audio32 source is MIT. JUCE is separately licensed under AGPLv3 or a commercial
JUCE license; distributing this optional linked plugin must satisfy that license.
See https://github.com/juce-framework/JUCE/blob/8.0.12/LICENSE.md.
