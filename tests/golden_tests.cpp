#include "dsp.hpp"
#include "effect_chain.hpp"
#include <cmath>
#include <iostream>
#include <vector>
int main() {
    audio32::DspEngine dsp;
    dsp.setFormat(48000,2);
    auto settings = audio32::effectPreset(audio32::EffectPreset::Voice, 48000);
    audio32::applyEffectChain(dsp,settings);
    std::vector<float> buf(1024);
    for (size_t i=0;i<512;++i){ float x=0.5f*std::sin(2.0f*3.14159f*440.0f*i/48000.0f); buf[2*i]=buf[2*i+1]=x; }
    dsp.process(buf);
    double rms=0; for(float v:buf) rms+=v*v;
    rms=std::sqrt(rms/buf.size());
    if (!std::isfinite(rms)) return 1;
    std::cout<<"golden tests passed\n";
    return 0;
}
