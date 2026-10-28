#include "dsp.hpp"
#include "effect_chain.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
int main() {
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> in(-1.0f,1.0f);
    std::uniform_real_distribution<float> p(0.0f,2.0f);
    for (int srate : {44100,48000,96000}) {
        for (int ch : {1,2}) {
            audio32::DspEngine dsp;
            dsp.setFormat(srate,ch);
            for (int iter=0; iter<100; ++iter) {
                audio32::EffectChainSettings st;
                st.gain = std::clamp(p(rng),0.0f,4.0f); st.ceiling = std::clamp(p(rng)*0.5f,0.01f,1.0f);
                st.equalizer = {std::clamp(p(rng),0.1f,4.0f),std::clamp(p(rng),0.1f,4.0f),std::clamp(p(rng),0.1f,4.0f),150,1000,6000,0.707f};
                st.compressor = {std::clamp(p(rng)*0.25f,0.001f,1.0f),std::clamp(p(rng)+1.0f,1.0f,100.0f),std::clamp(p(rng),0.0f,4.0f),10,100,6};
                st.reverb = {std::clamp(p(rng)*0.5f,0.0f,1.0f),std::clamp(p(rng)*0.5f,0.0f,0.95f),0.4f};
                st.delay = {std::clamp(p(rng)*0.5f,0.0f,1.0f),std::clamp(p(rng)*0.5f,0.0f,0.95f),static_cast<std::uint32_t>(srate*0.1)};
                audio32::applyEffectChain(dsp,st);
                std::vector<float> buf(512*ch);
                for (size_t i=0;i<buf.size();++i) buf[i]=in(rng);
                dsp.process(buf);
                for (float v:buf) if (!std::isfinite(v) || std::fabs(v)>10.0f) { std::cerr<<"bad sample\n"; return 1; }
            }
        }
    }
    std::cout<<"property tests passed\n";
    return 0;
}
