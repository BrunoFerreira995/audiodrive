#include "dsp.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    try {
        const unsigned iterations = argc > 1 ? std::stoul(argv[1]) : 10000;
        if (iterations < 100 || iterations > 1000000 || argc > 2)
            throw std::runtime_error("usage: audio32_latency_benchmark [iterations: 100..1000000]");
        std::cout << "profile,sample_rate,channels,frames,iterations,period_us,p50_us,p95_us,p99_us,max_us,deadline_misses,checksum\n";
        for (const auto rate : {44100,48000,96000})
        for (const auto frames : {64,128,256,512})
        for (const bool effects : {false,true}) {
            audio32::DspEngine dsp;
            dsp.setFormat(rate,2);
            if (effects) {
                dsp.setEqualizer({1.2F,0.8F,1.1F}); dsp.setCompressor({0.25F,4,1});
                dsp.setReverb({0.25F,0.7F}); dsp.setDelay({0.3F,0.5F,static_cast<unsigned>(rate/4)});
            }
            dsp.reset();
            std::vector<float> input(frames*2), block(frames*2);
            for (std::size_t i=0;i<input.size();++i) input[i]=0.3F*std::sin(i*0.07);
            std::vector<double> durations(iterations);
            double checksum=0;
            for (unsigned i=0;i<1000;++i) { block=input; dsp.process(block); }
            const double period=1e6*frames/rate;
            unsigned misses=0;
            for (unsigned i=0;i<iterations;++i) {
                std::copy(input.begin(),input.end(),block.begin());
                const auto start=std::chrono::steady_clock::now();
                dsp.process(block);
                const auto end=std::chrono::steady_clock::now();
                durations[i]=std::chrono::duration<double,std::micro>(end-start).count();
                misses+=durations[i]>period;
                checksum+=block[i%block.size()];
            }
            std::sort(durations.begin(),durations.end());
            const auto percentile=[&](double p) { return durations[static_cast<std::size_t>(std::ceil(p*iterations))-1]; };
            std::cout << (effects?"all_effects":"neutral") << ',' << rate << ",2," << frames << ',' << iterations << ','
                      << period << ',' << percentile(0.5) << ',' << percentile(0.95) << ',' << percentile(0.99) << ','
                      << durations.back() << ',' << misses << ',' << checksum << '\n';
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
