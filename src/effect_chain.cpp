#include "effect_chain.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
namespace audio32 {
EffectChainSettings effectPreset(EffectPreset preset, double rate) {
    EffectChainSettings s;
    if (preset==EffectPreset::Voice) { s.equalizer={0.7F,1.15F,1.05F}; s.compressor={0.25F,3,1.2F,5,120,6}; }
    if (preset==EffectPreset::Room) s.reverb={0.2F,0.65F,0.5F};
    if (preset==EffectPreset::Echo) s.delay={0.25F,0.4F,static_cast<std::uint32_t>((std::isfinite(rate)?std::clamp(rate,8000.0,384000.0):48000)*0.25)};
    return s;
}
EffectChainSettings captureEffectChain(const DspEngine& dsp) noexcept {
    return {dsp.gain(),dsp.limiterCeiling(),dsp.equalizer(),dsp.compressor(),dsp.reverb(),dsp.delay()};
}
void applyEffectChain(DspEngine& dsp,const EffectChainSettings& s,bool clearTails) {
    dsp.setGain(s.gain); dsp.setLimiterCeiling(s.ceiling); dsp.setEqualizer(s.equalizer);
    dsp.setCompressor(s.compressor); dsp.setReverb(s.reverb); dsp.setDelay(s.delay);
    if (clearTails) dsp.reset();
}
std::string serializeEffectChain(const EffectChainSettings& s) {
    std::ostringstream out; out.imbue(std::locale::classic()); out<<std::setprecision(std::numeric_limits<float>::max_digits10);
    out<<"audio32-chain 1\ngain "<<s.gain<<"\nceiling "<<s.ceiling
       <<"\neq "<<s.equalizer.lowGain<<' '<<s.equalizer.midGain<<' '<<s.equalizer.highGain<<' '<<s.equalizer.lowFrequency<<' '<<s.equalizer.midFrequency<<' '<<s.equalizer.highFrequency<<' '<<s.equalizer.midQ
       <<"\ncompressor "<<s.compressor.threshold<<' '<<s.compressor.ratio<<' '<<s.compressor.makeupGain<<' '<<s.compressor.attackMs<<' '<<s.compressor.releaseMs<<' '<<s.compressor.kneeDb
       <<"\nreverb "<<s.reverb.mix<<' '<<s.reverb.feedback<<' '<<s.reverb.damping
       <<"\ndelay "<<s.delay.mix<<' '<<s.delay.feedback<<' '<<s.delay.delayFrames<<'\n';
    return out.str();
}
std::optional<EffectChainSettings> deserializeEffectChain(std::string_view text) {
    if (text.size()>4096) return std::nullopt;
    std::istringstream in{std::string(text)}; in.imbue(std::locale::classic());
    std::string key; unsigned version=0; EffectChainSettings s;
    if (!(in>>key>>version) || key!="audio32-chain" || version!=1) return std::nullopt;
    const auto read=[&](const char* expected,auto&... values) { return bool(in>>key) && key==expected && bool((in>>...>>values)); };
    std::uint64_t delayFrames=0;
    if (!read("gain",s.gain) || !read("ceiling",s.ceiling) ||
        !read("eq",s.equalizer.lowGain,s.equalizer.midGain,s.equalizer.highGain,s.equalizer.lowFrequency,s.equalizer.midFrequency,s.equalizer.highFrequency,s.equalizer.midQ) ||
        !read("compressor",s.compressor.threshold,s.compressor.ratio,s.compressor.makeupGain,s.compressor.attackMs,s.compressor.releaseMs,s.compressor.kneeDb) ||
        !read("reverb",s.reverb.mix,s.reverb.feedback,s.reverb.damping) ||
        !read("delay",s.delay.mix,s.delay.feedback,delayFrames) || delayFrames>std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    in>>std::ws; if (!in.eof()) return std::nullopt;
    const float fields[]{s.gain,s.ceiling,s.equalizer.lowGain,s.equalizer.midGain,s.equalizer.highGain,s.equalizer.lowFrequency,s.equalizer.midFrequency,s.equalizer.highFrequency,s.equalizer.midQ,s.compressor.threshold,s.compressor.ratio,s.compressor.makeupGain,s.compressor.attackMs,s.compressor.releaseMs,s.compressor.kneeDb,s.reverb.mix,s.reverb.feedback,s.reverb.damping,s.delay.mix,s.delay.feedback};
    for (float value:fields) if (!std::isfinite(value)) return std::nullopt;
    s.delay.delayFrames=static_cast<std::uint32_t>(delayFrames);
    return s;
}
}
