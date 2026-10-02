#!/usr/bin/env python3
"""Capture raw benchmark CSV alongside the machine/build/source identity."""
import argparse
import datetime
import hashlib
import json
import pathlib
import platform
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--build-dir', default='build-benchmark')
parser.add_argument('--output', default='benchmarks/results/local')
parser.add_argument('--iterations', type=int, default=10000)
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
build = pathlib.Path(args.build_dir).resolve()
output = pathlib.Path(args.output).resolve()
output.parent.mkdir(parents=True, exist_ok=True)

def command(*parts):
    return subprocess.check_output(parts, cwd=root, text=True).strip()

cache = (build / 'CMakeCache.txt').read_text()
def cache_value(key):
    for line in cache.splitlines():
        if line.startswith(key + ':'):
            return line.split('=', 1)[1]
    return ''

if cache_value('CMAKE_BUILD_TYPE') != 'Release' or cache_value('AUDIO32_SANITIZER'):
    parser.error('benchmark requires a Release build without sanitizers')
compiler = cache_value('CMAKE_CXX_COMPILER')
metadata = {
    'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'platform': platform.platform(), 'architecture': platform.machine(),
    'cpu': command('sysctl', '-n', 'machdep.cpu.brand_string') if platform.system() == 'Darwin' else platform.processor(),
    'compiler': command(compiler, '--version'),
    'revision': command('git', 'rev-parse', 'HEAD'),
    'working_tree': command('git', 'status', '--short'),
    'iterations': args.iterations, 'warmup_blocks': 1000,
    'build_type': cache_value('CMAKE_BUILD_TYPE'),
    'cxx_flags': cache_value('CMAKE_CXX_FLAGS'),
    'release_flags': cache_value('CMAKE_CXX_FLAGS_RELEASE'),
    'binary_sha256': hashlib.sha256((build / 'audio32_latency_benchmark').read_bytes()).hexdigest(),
    'source_sha256': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in [root / 'src/dsp.cpp', root / 'include/dsp.hpp', root / 'benchmarks/dsp_latency.cpp']},
    'measurement': 'steady_clock wall time around DspEngine::process only; stereo; synthetic input; unpaced',
}
result = command(str(build / 'audio32_latency_benchmark'), str(args.iterations))
output.with_suffix('.csv').write_text(result + '\n')
output.with_suffix('.json').write_text(json.dumps(metadata, indent=2) + '\n')
print(f'Wrote {output.with_suffix(".csv")} and {output.with_suffix(".json")}')
