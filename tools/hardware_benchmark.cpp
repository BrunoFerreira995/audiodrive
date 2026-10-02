#include "native_audio.hpp"
#include "roundtrip_analysis.hpp"
#include "buffer.hpp"
#include "effect_chain.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

using Clock=std::chrono::steady_clock;
int main(int argc,char** argv) {
    try {
        audio32::NativeAudioBackend backend;
        if (argc==2 && std::string(argv[1])=="--list") {
            auto devices=backend.devices();
            for (const auto& device:devices)
                std::cout<<(device.capture?"input":"output")<<'\t'<<device.id<<'\t'<<device.name<<(device.isDefault?" [default]":"")<<'\n';
            if (devices.empty()) { std::cerr<<"No devices; backend error "<<backend.lastError()<<'\n'; return 1; }
            return 0;
        }
        if (argc<5) throw std::runtime_error("usage: --list OR --playback seconds load_threads report.csv [output_id] OR --roundtrip seconds 0 report.csv [output_id] [input_id]");
        const bool roundtrip=std::string(argv[1])=="--roundtrip";
        if (!roundtrip && std::string(argv[1])!="--playback") throw std::runtime_error("unknown mode");
        const unsigned seconds=std::stoul(argv[2]),loads=std::stoul(argv[3]);
        if (seconds<3 || seconds>3600 || loads>64 || (roundtrip && loads!=0)) throw std::runtime_error("seconds 3..3600; load threads 0..64; roundtrip requires zero load");
        audio32::NativeAudioConfig config;
        config.capture=roundtrip; if (argc>5) config.outputDeviceId=argv[5]; if (argc>6) config.inputDeviceId=argv[6];
        const std::size_t total=static_cast<std::size_t>(seconds)*config.sampleRate;
        if (roundtrip && seconds>60) throw std::runtime_error("roundtrip duration capped at 60 seconds");
        audio32::AudioBuffer ring(8192*sizeof(float));
        audio32::DspEngine dsp; dsp.setFormat(config.sampleRate,2);
        auto settings=audio32::effectPreset(audio32::EffectPreset::Voice); settings.reverb={0.2F,0.6F,0.4F}; settings.delay={0.2F,0.4F,12000};
        audio32::applyEffectChain(dsp,settings,true);
        std::vector<float> capture(roundtrip?total:0,0),code(511);
        std::mt19937 random(42); for (auto& x:code) x=(random()&1)?0.03F:-0.03F;
        std::vector<double> times(total/16+1000),budgets(times.size());
        std::size_t callbacks=0,position=0,starvations=0,inputMissing=0;
        std::atomic<bool> done{false};
        std::atomic<double> loadChecksum{0};
        const auto callback=[&](float* output,const float* input,unsigned frames,unsigned channels) {
            const auto start=Clock::now();
            if (roundtrip) {
                for (unsigned i=0;i<frames;++i) {
                    const auto at=position+i;
                    // A reproducible burst each second, starting half a second into the run.
                    const auto relative=(at+config.sampleRate/2)%config.sampleRate;
                    const float x=relative<code.size()?code[relative]:0;
                    for (unsigned ch=0;ch<channels;++ch) output[i*channels+ch]=x;
                    if (at<total) capture[at]=input?input[i*channels]:0;
                }
                if (!input) ++inputMissing;
            } else {
                auto span=std::span<float>(output,frames*channels);
                if (ring.read(span)<span.size()) ++starvations;
                dsp.process(span);
            }
            position+=frames;
            if (callbacks<times.size()) {
                times[callbacks]=std::chrono::duration<double,std::micro>(Clock::now()-start).count();
                budgets[callbacks]=1e6*frames/config.sampleRate;
                ++callbacks;
            }
        };
        if (!backend.initialize(config,callback)) throw std::runtime_error("device initialization failed: "+std::to_string(backend.lastError()));
        // Prepare producer before start so priming is outside the measurement.
        std::thread producer;
        if (!roundtrip) {
            std::vector<float> prime(8192,0); ring.write(prime);
            producer=std::thread([&] {
                std::vector<float> block(1024); std::uint64_t generated=0;
                while (!done.load()) {
                    for (unsigned i=0;i<512;++i) block[2*i]=block[2*i+1]=0.02F*std::sin(6.283185307179586*440*(generated+i)/config.sampleRate);
                    std::size_t sent=0;
                    while (sent<block.size() && !done.load()) {
                        const auto n=ring.write(std::span<const float>(block).subspan(sent)); sent+=n;
                        if (!n) std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    generated+=512;
                }
            });
        }
        std::vector<std::thread> workers;
        for (unsigned i=0;i<loads;++i) workers.emplace_back([&,i] {
            double x=0.1+i;
            while (!done.load(std::memory_order_relaxed)) for (unsigned n=0;n<4096;++n) x=std::sin(x)+0.001*n;
            loadChecksum.fetch_add(x);
        });
        const bool started=backend.start();
        if (started) std::this_thread::sleep_for(std::chrono::seconds(seconds));
        backend.stop(); done=true;
        if (producer.joinable()) producer.join(); for (auto& worker:workers) worker.join();
        if (!started) throw std::runtime_error("device start failed: "+std::to_string(backend.lastError()));
        if (!callbacks) throw std::runtime_error("no hardware callbacks observed");
        std::ofstream report(argv[4]); if (!report) throw std::runtime_error("cannot open report");
        if (roundtrip) {
            if (inputMissing) throw std::runtime_error("input callback had no capture data");
            report<<"burst,lag_frames,roundtrip_ms,correlation\n";
            unsigned valid=0;
            for (std::size_t burst=config.sampleRate/2;burst+config.sampleRate+code.size()<std::min(position,total);burst+=config.sampleRate) {
                const auto matched=audio32::diagnostics::findRoundTrip(std::span<const float>(capture).subspan(burst),code,config.sampleRate);
                if (matched) {
                    report<<burst<<','<<matched->lag<<','<<1000.0*matched->lag/config.sampleRate<<','<<matched->correlation<<'\n';
                    ++valid;
                }
            }
            if (!valid) throw std::runtime_error("no confident loopback signal; no physical latency result (connect an output-to-input cable)");
        } else {
            std::size_t overruns=0; for (std::size_t i=0;i<callbacks;++i) overruns+=times[i]>budgets[i];
            times.resize(callbacks); std::sort(times.begin(),times.end());
            const auto percentile=[&](double p) { return times[static_cast<std::size_t>(std::ceil(p*callbacks))-1]; };
            report<<"backend,rate,channels,requested_period,seconds,load_threads,callbacks,buffer_starvations,callback_budget_overruns,p50_us,p99_us,max_us,backend_error,load_checksum\n";
            report<<backend.backendName()<<",48000,2,128,"<<seconds<<','<<loads<<','<<callbacks<<','<<starvations<<','<<overruns<<','<<percentile(0.5)<<','<<percentile(0.99)<<','<<times.back()<<','<<backend.lastError()<<','<<loadChecksum.load()<<'\n';
        }
        std::cout<<"Recorded "<<argv[4]<<'\n';
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
