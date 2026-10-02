#include "audio_io.hpp"
#include "effect_chain.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
void integer(std::ostream& out,std::uint32_t value,unsigned bytes) {
    for(unsigned i=0;i<bytes;++i) out.put(static_cast<char>((value>>(i*8))&255));
}
void wav(const std::filesystem::path& path,const audio32::DecodedAudio& audio) {
    const auto bytes=audio.samples.size()*2;
    if(bytes>0xffffffffU-36) throw std::runtime_error("WAV exceeds RIFF size");
    std::ofstream out(path,std::ios::binary); if(!out) throw std::runtime_error("cannot open output");
    out.write("RIFF",4); integer(out,36+bytes,4); out.write("WAVEfmt ",8); integer(out,16,4);
    integer(out,1,2); integer(out,audio.channels,2); integer(out,audio.sampleRate,4);
    integer(out,audio.sampleRate*audio.channels*2,4); integer(out,audio.channels*2,2); integer(out,16,2);
    out.write("data",4); integer(out,bytes,4);
    for(float x:audio.samples) integer(out,static_cast<std::uint16_t>(static_cast<std::int16_t>(std::clamp(x,-1.0F,1.0F)*32767)),2);
    if(!out) throw std::runtime_error("WAV write failed");
}
int main(int argc,char** argv) {
    try {
        if(argc!=3) throw std::runtime_error("usage: audio32_render_effects input-audio output-directory");
        auto source=audio32::decodeAudioFile(argv[1]); if(!source) throw std::runtime_error("decode failed");
        std::filesystem::create_directories(argv[2]);
        source->samples.resize(source->samples.size()+static_cast<std::size_t>(source->sampleRate*4)*source->channels,0);
        for(const std::string name:{"dry","equalizer","compressor","reverb","delay","voice","room","echo"}) {
            audio32::EffectChainSettings settings;
            if(name=="equalizer") settings.equalizer={0.7F,1.4F,1.2F};
            if(name=="compressor") settings.compressor={0.1F,4,1.5F,5,120,6};
            if(name=="reverb") settings.reverb={0.3F,0.7F,0.4F};
            if(name=="delay") settings.delay={0.3F,0.5F,static_cast<std::uint32_t>(source->sampleRate*0.25)};
            if(name=="voice") settings=audio32::effectPreset(audio32::EffectPreset::Voice,source->sampleRate);
            if(name=="room") settings=audio32::effectPreset(audio32::EffectPreset::Room,source->sampleRate);
            if(name=="echo") settings=audio32::effectPreset(audio32::EffectPreset::Echo,source->sampleRate);
            audio32::DspEngine dsp; dsp.setFormat(source->sampleRate,source->channels);
            audio32::applyEffectChain(dsp,settings,true);
            auto audio=*source; dsp.process(audio.samples);
            auto prefix=std::filesystem::path(argv[2])/name;
            wav(prefix.string()+".wav",audio);
            std::ofstream(prefix.string()+".chain")<<audio32::serializeEffectChain(audio32::captureEffectChain(dsp));
        }
        std::cout<<"Rendered dry reference, four isolated effects, and three presets\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
