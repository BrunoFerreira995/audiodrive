#include "effect_chain.hpp"
#include "../tools/roundtrip_analysis.hpp"
#include <random>
#include "audio_driver.hpp"
#include "input_capture.hpp"
#include "native_audio.hpp"
#include "audio_io.hpp"
#include <stdexcept>
#include <iostream>
#include <cmath>
void check(bool condition,const char* message) { if (!condition) throw std::runtime_error(message); }
int main(int argc,char** argv) {
    for (auto preset:{audio32::EffectPreset::Neutral,audio32::EffectPreset::Voice,audio32::EffectPreset::Room,audio32::EffectPreset::Echo}) {
        auto settings=audio32::effectPreset(preset,96000);
        auto text=audio32::serializeEffectChain(settings);
        auto decoded=audio32::deserializeEffectChain(text);
        check(decoded.has_value(),"preset decode");
        check(audio32::serializeEffectChain(*decoded)==text,"lossless chain roundtrip");
        audio32::DspEngine dsp; dsp.setFormat(96000,2); audio32::applyEffectChain(dsp,*decoded,true);
        check(audio32::serializeEffectChain(audio32::captureEffectChain(dsp))==text,"apply preset");
        check(!audio32::deserializeEffectChain(text+"junk"),"reject trailing fields");
        check(!audio32::deserializeEffectChain(text.substr(0,text.size()/2)),"reject incomplete chain");
    }
    check(!audio32::deserializeEffectChain("audio32-chain 2"),"reject unknown version");
    {
        std::mt19937 random(42); std::vector<float> probe(511),input(2048,0);
        for (auto& x:probe) x=(random()&1)?0.03F:-0.03F;
        for (std::size_t i=0;i<probe.size();++i) input[137+i]=-probe[i]*0.5F;
        auto match=audio32::diagnostics::findRoundTrip(input,probe,1000);
        check(match && match->lag==137 && match->correlation>0.99,"physical lag estimator finds inverted attenuated signal");
        check(!audio32::diagnostics::findRoundTrip(std::vector<float>(2048,0),probe,1000),"no false latency from silence");
        check(!audio32::diagnostics::findRoundTrip(input,probe,100),"reject signal outside lag window");
    }
    audio32::AudioDriver driver(7*sizeof(float)); driver.setChannels(3);
    check(driver.write(std::vector<float>(9,0.1F))==6,"driver writes only complete frames");
    audio32::AudioInputCapture capture;
    check(!capture.initialize(48000,0),"reject zero-channel capture");
    audio32::NativeAudioBackend backend;
    audio32::NativeAudioConfig invalid; invalid.channels=0;
    check(!backend.initialize(invalid,[](float*,const float*,unsigned,unsigned){}),"reject invalid hardware format");
    check(!backend.start() && !backend.isRunning(),"cannot start uninitialized device");
    check(!audio32::decodeAudioFile("/nonexistent/audio32.mp3"),"missing compressed file");
    for (int i=1;i<argc;++i) {
        auto audio=audio32::decodeAudioFile(argv[i]);
        check(audio && audio->channels==1 && audio->sampleRate==48000,"compressed fixture format");
        check(audio->samples.size()>4000,"compressed fixture samples");
        double energy=0; for(float x:audio->samples) { check(std::isfinite(x),"finite decode"); energy+=x*x; }
        check(energy>1,"non-silent compressed decode");
    }
    std::cout<<"Phase 6 tests passed\n";
}
