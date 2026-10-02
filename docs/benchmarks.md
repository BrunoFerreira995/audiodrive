# Repeatable DSP latency benchmarks

This benchmark times `DspEngine::process()` with `steady_clock`, comparing wall
clock processing duration with the buffer deadline (`frames / sample rate`).
It measures compute latency only. AudioQueue scheduling, queue depth, device
buffers, DAC latency, and physical round-trip latency are outside this measurement.
No hardware latency or sub-5-ms playback claim follows from these numbers.

## Reproduce

Run from the repository root with CMake, a C++20 compiler, and Python 3:

```sh
cmake -S . -B build-benchmark -DCMAKE_BUILD_TYPE=Release -DAUDIO32_BUILD_BENCHMARKS=ON
cmake --build build-benchmark --parallel 4
python3 benchmarks/run.py --build-dir build-benchmark --iterations 10000 --output benchmarks/results/local
```

The runner rejects non-Release and sanitizer builds. It writes CSV and JSON with
UTC timestamp, CPU, OS, architecture, compiler, optimization flags, git revision,
working-tree status, source hashes, and executable hash. On macOS, CPU identification
uses `sysctl`. Keep the metadata with the CSV, especially for uncommitted builds.

The matrix uses stereo, 44.1/48/96 kHz, and 64/128/256/512 frames. Profiles are
neutral settings and all effects enabled (EQ 1.2/0.8/1.1; compressor threshold 0.25,
ratio 4; reverb mix 0.25, feedback 0.7; delay mix 0.3, feedback 0.5, 250 ms).
Each case warms up for 1,000 blocks and measures 10,000 blocks. Input is a fixed
synthetic sine sequence repeated each block. Copying input, sorting, allocation,
and output formatting are outside the timed interval. A reported checksum keeps
processed output observable. Percentiles use nearest-rank selection.

For comparisons, use the same machine, power mode, compiler, and flags. Close
background workloads and repeat runs at least three times; retain all results.
Runs are unpaced, execute cases in a fixed order, and include clock overhead and
scheduler interruptions. They do not simulate callback scheduling or guarantee
absence of underruns. A deadline miss means measured DSP time exceeded the
nominal buffer period, not an observed hardware dropout. Do not use shared CI
runner timings as a hard performance regression threshold.

## Recorded baseline

[Raw CSV](../benchmarks/results/2026-10-02-local.csv) and
[machine/build metadata](../benchmarks/results/2026-10-02-local.json):
Apple M4, arm64, macOS 27.0.1, AppleClang 21.0.0, Release `-O3 -DNDEBUG`,
10,000 measured blocks per case. This baseline includes the uncommitted DSP
implementation identified by the metadata's source hashes.

48 kHz, stereo, all effects enabled (microseconds):

| Frames | Buffer period | p50 | p95 | p99 | Maximum | DSP deadline misses |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 1333.33 | 2.583 | 3.250 | 3.292 | 42.791 | 0 |
| 128 | 2666.67 | 5.209 | 6.458 | 6.625 | 40.500 | 0 |
| 256 | 5333.33 | 10.416 | 12.500 | 13.209 | 95.083 | 0 |
| 512 | 10666.70 | 20.709 | 24.625 | 26.458 | 90.375 | 0 |

The full CSV contains both profiles and all sample rates. A single local run
establishes a reproducible baseline, not a cross-machine performance guarantee.

## Automated workflows

`.github/workflows/validation.yml` runs tests on pushes, pull requests, manual
requests, and a weekly schedule. The matrix covers macOS and Linux ASan/UBSan,
macOS ThreadSanitizer, and an optimized macOS build. Sanitizer reports fail the
job. Stress tests additionally repeat ten times with a 120-second per-run timeout.

`.github/workflows/benchmarks.yml` is manually dispatched on macOS and uploads
raw CSV plus metadata as a GitHub Actions artifact. It has no timing pass/fail gate.
The workflows take effect after these files reach the remote repository.

## Local sanitizers and stress

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAUDIO32_SANITIZER=address-undefined
cmake --build build-asan --parallel 4
ASAN_OPTIONS=halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-asan --output-on-failure

cmake -S . -B build-tsan -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAUDIO32_SANITIZER=thread
cmake --build build-tsan --parallel 4
TSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-tsan --output-on-failure
ctest --test-dir build-tsan -L stress --repeat until-fail:10 --output-on-failure
```

ASan/UBSan and TSan use separate builds. Existing assertion-based tests stay
active in optimized builds. The stress suite checks one million exact ordered
samples across concurrent producer/consumer threads in a small wraparound ring,
then exercises deterministic randomized DSP parameters, variable block sizes,
resets, five sample rates, and one/two/eight channels. It respects the SPSC and DSP
exclusive-configuration contracts; it does not exercise physical audio devices.
Local Release, ASan/UBSan, and TSan suites passed on the recorded machine.
