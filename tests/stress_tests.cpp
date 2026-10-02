#include "buffer.hpp"
#include "dsp.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

int main() {
    // A deliberately small, non-power-of-two ring forces repeated wraparound.
    audio32::AudioBuffer ring(1031*sizeof(float));
    constexpr std::size_t count=1000000;
    std::atomic<bool> failed{false};
    std::thread producer([&] {
        std::vector<float> block(137);
        std::size_t sent=0;
        while (sent<count) {
            const auto n=std::min(block.size(),count-sent);
            for (std::size_t i=0;i<n;++i) block[i]=static_cast<float>(sent+i);
            const auto written=ring.write(std::span<const float>(block).first(n));
            sent+=written;
            if (!written) std::this_thread::yield();
        }
    });
    std::thread consumer([&] {
        std::vector<float> block(193);
        std::size_t received=0;
        while (received<count) {
            const auto read=ring.read(block);
            for (std::size_t i=0;i<read;++i)
                if (block[i]!=static_cast<float>(received+i)) failed=true;
            received+=read;
            if (!read) std::this_thread::yield();
        }
    });
    producer.join(); consumer.join();
    if (failed || ring.availableRead()!=0) throw std::runtime_error("SPSC sample ordering failed");
    std::mt19937 random(0xA032);
    for (auto rate:{8000,44100,48000,96000,192000})
    for (auto channels:{1U,2U,8U}) {
        audio32::DspEngine dsp; dsp.setFormat(rate,channels);
        std::vector<float> block(257*channels);
        for (unsigned iteration=0;iteration<200;++iteration) {
            if (iteration%11==0) {
                const auto unit=[&] { return static_cast<float>(random()%1000)/1000; };
                dsp.setEqualizer({0.001F+16*unit(),0.001F+16*unit(),0.001F+16*unit(),20+rate*unit(),20+rate*unit(),20+rate*unit(),0.1F+10*unit()});
                dsp.setCompressor({0.001F+unit(),1+99*unit(),unit(),0.01F+unit()*1000,1+unit()*10000,24*unit()});
                dsp.setReverb({unit(),unit(),unit()});
                dsp.setDelay({unit(),unit(),static_cast<unsigned>(random()%(rate*3))});
            }
            const auto frames=1+random()%257;
            for (auto& sample:block) sample=(static_cast<int>(random()%2001)-1000)*0.001F;
            dsp.process(std::span<float>(block).first(frames*channels));
            for (std::size_t i=0;i<frames*channels;++i)
                if (!std::isfinite(block[i]) || std::fabs(block[i])>1)
                    throw std::runtime_error("DSP stress produced invalid output");
            if (iteration%47==0) dsp.reset();
        }
    }
    std::cout << "SPSC and DSP stress passed\n";
}
