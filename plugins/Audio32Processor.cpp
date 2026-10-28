#include <juce_audio_processors/juce_audio_processors.h>
#include "effect_chain.hpp"
#include "midi.hpp"
#include <array>

class Audio32Processor final : public juce::AudioProcessor {
public:
    Audio32Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                      .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
        parameters(*this,nullptr,"Audio32",layout()) {
        for (std::size_t i=0;i<ids.size();++i) values[i]=parameters.getRawParameterValue(ids[i]);
        bypassValue = parameters.getRawParameterValue("bypass");
    }
    const juce::String getName() const override { return "Audio32"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    int getLatencySamples() const override { return latencySamples; }
    double getTailLengthSeconds() const override {
        const auto repeats=[](float feedback) { return feedback>0 ? std::max(1.0,std::log(0.001)/std::log(std::min(0.95F,feedback))) : 1.0; };
        const double reverb=values[15]->load()>0 ? 0.06*repeats(values[16]->load())+0.03 : 0;
        const double delay=values[18]->load()>0 ? values[20]->load()*repeats(values[19]->load()) : 0;
        return std::ceil(reverb+delay);
    }
    int getNumPrograms() override { return 4; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override {
        currentProgram = juce::jlimit(0, getNumPrograms()-1, index);
        applyPreset(static_cast<audio32::EffectPreset>(currentProgram));
    }
    const juce::String getProgramName(int index) override {
        switch (index) {
            case 0: return "Neutral";
            case 1: return "Voice";
            case 2: return "Room";
            case 3: return "Echo";
            default: return "Default";
        }
    }
    void changeProgramName(int,const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout& buses) const override {
        const auto output=buses.getMainOutputChannelSet();
        return (output==juce::AudioChannelSet::mono() || output==juce::AudioChannelSet::stereo()) && output==buses.getMainInputChannelSet();
    }
    void prepareToPlay(double sampleRate,int maximumBlock) override {
        rate=sampleRate; channels=static_cast<unsigned>(getTotalNumOutputChannels());
        capacity=static_cast<unsigned>(std::max(1,maximumBlock));
        scratch.resize(capacity*channels);
        dsp.setFormat(rate,channels);
        latencySamples = 0;
        update(); dsp.reset();
    }
    void releaseResources() override { dsp.reset(); }
    void reset() override { dsp.reset(); }

    void applyPresetSettings(const audio32::EffectChainSettings& settings) {
        const auto setParam = [&](const char* id, float value) {
            if (auto* param = parameters.getParameter(id)) {
                param->setValueNotifyingHost(param->convertTo0to1(value));
            }
        };
        setParam("gain", settings.gain);
        setParam("ceiling", settings.ceiling);
        setParam("eqLow", settings.equalizer.lowGain);
        setParam("eqMid", settings.equalizer.midGain);
        setParam("eqHigh", settings.equalizer.highGain);
        setParam("lowHz", settings.equalizer.lowFrequency);
        setParam("midHz", settings.equalizer.midFrequency);
        setParam("highHz", settings.equalizer.highFrequency);
        setParam("midQ", settings.equalizer.midQ);
        setParam("threshold", settings.compressor.threshold);
        setParam("ratio", settings.compressor.ratio);
        setParam("makeup", settings.compressor.makeupGain);
        setParam("attack", settings.compressor.attackMs);
        setParam("release", settings.compressor.releaseMs);
        setParam("knee", settings.compressor.kneeDb);
        setParam("reverbMix", settings.reverb.mix);
        setParam("reverbFeedback", settings.reverb.feedback);
        setParam("damping", settings.reverb.damping);
        setParam("delayMix", settings.delay.mix);
        setParam("delayFeedback", settings.delay.feedback);
        setParam("delaySeconds", static_cast<float>(settings.delay.delayFrames) / static_cast<float>(rate > 0 ? rate : 48000.0));
        dsp.setGain(settings.gain); dsp.setLimiterCeiling(settings.ceiling);
        dsp.setEqualizer(settings.equalizer); dsp.setCompressor(settings.compressor);
        dsp.setReverb(settings.reverb); dsp.setDelay(settings.delay);
    }

    void applyPreset(audio32::EffectPreset preset) {
        applyPresetSettings(audio32::effectPreset(preset, rate));
    }

    void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) override {
        juce::ScopedNoDenormals noDenormals;
        if (!capacity || buffer.getNumChannels()!=static_cast<int>(channels)) { buffer.clear(); return; }

        for (const auto metadata : midi) {
            const auto msg = metadata.getMessage();
            if (msg.isController()) {
                const std::uint8_t cc = static_cast<std::uint8_t>(msg.getControllerNumber());
                const std::uint8_t val = static_cast<std::uint8_t>(msg.getControllerValue());
                if (ccLearnActive && ccLearnTarget >= 0 && ccLearnTarget < 21) {
                    audio32::MidiControlMapping mapping;
                    mapping.controller = cc;
                    mapping.target = mapToMidiTarget(ccLearnTarget);
                    mapping.minimum = 0.0F;
                    mapping.maximum = 1.0F;
                    midiMapper.mapControl(mapping);
                    ccLearnActive = false;
                    continue;
                }
                if (auto m = midiMapper.mappingFor(cc)) {
                    const auto scaled = static_cast<float>(val) / 127.0F;
                    switch (m->target) {
                        case audio32::MidiTarget::Gain: { float v = juce::jmap(scaled, 0.0f, 4.0f); if (auto* p = parameters.getParameter("gain")) p->setValueNotifyingHost(p->convertTo0to1(v)); break; }
                        case audio32::MidiTarget::LimiterCeiling: { float v = juce::jmap(scaled, 0.01f, 1.0f); if (auto* p = parameters.getParameter("ceiling")) p->setValueNotifyingHost(p->convertTo0to1(v)); break; }
                        case audio32::MidiTarget::EqualizerLowGain: { float v = juce::jmap(scaled, 0.1f, 4.0f); if (auto* p = parameters.getParameter("eqLow")) p->setValueNotifyingHost(p->convertTo0to1(v)); break; }
                        case audio32::MidiTarget::EqualizerMidGain: { float v = juce::jmap(scaled, 0.1f, 4.0f); if (auto* p = parameters.getParameter("eqMid")) p->setValueNotifyingHost(p->convertTo0to1(v)); break; }
                        case audio32::MidiTarget::EqualizerHighGain: { float v = juce::jmap(scaled, 0.1f, 4.0f); if (auto* p = parameters.getParameter("eqHigh")) p->setValueNotifyingHost(p->convertTo0to1(v)); break; }
                        case audio32::MidiTarget::CompressorThreshold: { float v = juce::jmap(scaled, 0.001f, 1.0f); if (auto* p = parameters.getParameter("threshold")) p->setValueNotifyingHost(p->convertTo0to1(v)); break; }
                        case audio32::MidiTarget::CompressorRatio: { float v = juce::jmap(scaled, 1.0f, 100.0f); if (auto* p = parameters.getParameter("ratio")) p->setValueNotifyingHost(p->convertTo0to1(v)); break; }
                        case audio32::MidiTarget::ReverbMix: { float v = scaled; if (auto* p = parameters.getParameter("reverbMix")) p->setValueNotifyingHost(v); break; }
                        case audio32::MidiTarget::DelayMix: { float v = scaled; if (auto* p = parameters.getParameter("delayMix")) p->setValueNotifyingHost(v); break; }
                    }
                }
            }
        }

        const bool bypassed = bypassValue && bypassValue->load(std::memory_order_relaxed) > 0.5f;
        if (bypassed) {
            return;
        }

        update();
        const auto total=static_cast<unsigned>(buffer.getNumSamples());
        for (unsigned offset=0;offset<total;offset+=capacity) {
            const auto frames=std::min(capacity,total-offset);
            for (unsigned ch=0;ch<channels;++ch)
                for (unsigned frame=0;frame<frames;++frame) scratch[frame*channels+ch]=buffer.getSample(ch,offset+frame);
            dsp.process(std::span<float>(scratch).first(frames*channels));
            for (unsigned ch=0;ch<channels;++ch)
                for (unsigned frame=0;frame<frames;++frame) buffer.setSample(ch,offset+frame,scratch[frame*channels+ch]);
        }
    }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock& destination) override {
        if (auto xml=parameters.copyState().createXml()) copyXmlToBinary(*xml,destination);
    }
    void setStateInformation(const void* data,int size) override {
        if (auto xml=getXmlFromBinary(data,size))
            if (xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
private:
    friend class Audio32Editor;
    int currentProgram = 0;
    int latencySamples = 0;
    std::atomic<float>* bypassValue = nullptr;
    audio32::MidiMapper midiMapper;
    bool ccLearnActive = false;
    int ccLearnTarget = -1;
    float lastGain=1,lastCeil=1,lastEqQ=0.707f,lastEqK=0,lastCompK=6,lastDelT=0.25f;
    std::array<float,3> lastEq{1,1,1}, lastEqF{150,1000,6000}, lastComp{1,1,1}, lastCompT{10,100}, lastRev{0,0.25f,0.4f}, lastDel{0,0.25f};
    audio32::MidiTarget mapToMidiTarget(int idx) {
        switch (idx) {
            case 0: return audio32::MidiTarget::Gain;
            case 1: return audio32::MidiTarget::LimiterCeiling;
            case 2: case 5: return audio32::MidiTarget::EqualizerLowGain;
            case 3: case 6: return audio32::MidiTarget::EqualizerMidGain;
            case 4: case 7: return audio32::MidiTarget::EqualizerHighGain;
            case 9: return audio32::MidiTarget::CompressorThreshold;
            case 10: return audio32::MidiTarget::CompressorRatio;
            case 15: return audio32::MidiTarget::ReverbMix;
            case 18: return audio32::MidiTarget::DelayMix;
            default: return audio32::MidiTarget::Gain;
        }
    }
    static constexpr std::array<const char*,21> ids={"gain","ceiling","eqLow","eqMid","eqHigh","lowHz","midHz","highHz","midQ","threshold","ratio","makeup","attack","release","knee","reverbMix","reverbFeedback","damping","delayMix","delayFeedback","delaySeconds"};
    static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
        juce::AudioProcessorValueTreeState::ParameterLayout result;
        const auto add=[&](const char* id,const char* name,float low,float high,float initial,
                            const juce::String& unit="", std::function<juce::String(float)> text=nullptr) {
            auto range = juce::NormalisableRange<float>(low,high);
            auto param = std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,range,initial,
                juce::AudioParameterFloatAttributes().withLabel(unit));
            if (text) {
                param->setTextFunction([text](float v, int){ return text(v); });
            }
            result.add(std::move(param));
        };
        result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));
        add("gain","Gain",0,4,1,"x",[](float v){ return juce::String(v,2)+"x"; });
        add("ceiling","Output ceiling",0.01F,1,1,"",[](float v){ return juce::String(v,2); });
        add("eqLow","Low gain",0.1F,4,1,"x",[](float v){ return juce::String(v,2)+"x"; });
        add("eqMid","Mid gain",0.1F,4,1,"x",[](float v){ return juce::String(v,2)+"x"; });
        add("eqHigh","High gain",0.1F,4,1,"x",[](float v){ return juce::String(v,2)+"x"; });
        add("lowHz","Low frequency",20,2000,150,"Hz",[](float v){ return juce::String(static_cast<int>(v))+" Hz"; });
        add("midHz","Mid frequency",20,20000,1000,"Hz",[](float v){ return juce::String(static_cast<int>(v))+" Hz"; });
        add("highHz","High frequency",1000,20000,6000,"Hz",[](float v){ return juce::String(static_cast<int>(v))+" Hz"; });
        add("midQ","Mid Q",0.1F,10,0.707F,"",[](float v){ return juce::String(v,3); });
        add("threshold","Threshold",0.001F,1,1,"",[](float v){ return juce::String(v,3); });
        add("ratio","Ratio",1,100,1,"",[](float v){ return juce::String(v,2)+":1"; });
        add("makeup","Makeup gain",0,4,1,"x",[](float v){ return juce::String(v,2)+"x"; });
        add("attack","Attack",0.01F,1000,10,"ms",[](float v){ return juce::String(v,2)+" ms"; });
        add("release","Release",1,10000,100,"ms",[](float v){ return juce::String(v,0)+" ms"; });
        add("knee","Knee",0,24,6,"dB",[](float v){ return juce::String(v,1)+" dB"; });
        add("reverbMix","Reverb mix",0,1,0,"%",[](float v){ return juce::String(static_cast<int>(v*100))+"%"; });
        add("reverbFeedback","Reverb feedback",0,0.95F,0.25F,"",[](float v){ return juce::String(v,2); });
        add("damping","Reverb damping",0,0.99F,0.4F,"",[](float v){ return juce::String(v,2); });
        add("delayMix","Delay mix",0,1,0,"%",[](float v){ return juce::String(static_cast<int>(v*100))+"%"; });
        add("delayFeedback","Delay feedback",0,0.95F,0.25F,"",[](float v){ return juce::String(v,2); });
        add("delaySeconds","Delay time",0,2,0.25F,"s",[](float v){ return juce::String(v,3)+" s"; });
        return result;
    }
    void update() {
        const auto v=[&](std::size_t i) { return values[i]->load(std::memory_order_relaxed); };
        const float gv=v(0), cv=v(1);
        const float eqlow=v(2), eqmid=v(3), eqhigh=v(4), elow=v(5), emid=v(6), ehigh=v(7), eq=v(8);
        const float ct=v(9), cr=v(10), cm=v(11), cat=v(12), crel=v(13), ck=v(14);
        const float rm=v(15), rf=v(16), rd=v(17);
        const float dm=v(18), df=v(19), ds=v(20);
        if (std::fabs(gv-lastGain)<1e-6f && std::fabs(cv-lastCeil)<1e-6f &&
            std::fabs(eqlow-lastEq[0])<1e-6f && std::fabs(eqmid-lastEq[1])<1e-6f && std::fabs(eqhigh-lastEq[2])<1e-6f &&
            std::fabs(elow-lastEqF[0])<1e-3f && std::fabs(emid-lastEqF[1])<1e-3f && std::fabs(ehigh-lastEqF[2])<1e-3f && std::fabs(eq-lastEqQ)<1e-6f &&
            std::fabs(ct-lastComp[0])<1e-6f && std::fabs(cr-lastComp[1])<1e-6f && std::fabs(cm-lastComp[2])<1e-6f &&
            std::fabs(cat-lastCompT[0])<1e-6f && std::fabs(crel-lastCompT[1])<1e-6f && std::fabs(ck-lastCompK)<1e-6f &&
            std::fabs(rm-lastRev[0])<1e-6f && std::fabs(rf-lastRev[1])<1e-6f && std::fabs(rd-lastRev[2])<1e-6f &&
            std::fabs(dm-lastDel[0])<1e-6f && std::fabs(df-lastDel[1])<1e-6f && std::fabs(ds-lastDelT)<1e-6f)
            return;
        lastGain=gv; lastCeil=cv; lastEq={eqlow,eqmid,eqhigh}; lastEqF={elow,emid,ehigh}; lastEqQ=eq;
        lastComp={ct,cr,cm}; lastCompT={cat,crel}; lastCompK=ck; lastRev={rm,rf,rd}; lastDel={dm,df}; lastDelT=ds;
        audio32::EffectChainSettings settings;
        settings.gain=gv; settings.ceiling=cv;
        settings.equalizer={eqlow,eqmid,eqhigh,elow,emid,ehigh,eq};
        settings.compressor={ct,cr,cm,cat,crel,ck};
        settings.reverb={rm,rf,rd};
        settings.delay={dm,df,static_cast<std::uint32_t>(ds*rate)};
        audio32::applyEffectChain(dsp,settings);
    }
    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*,21> values{};
    audio32::DspEngine dsp;
    std::vector<float> scratch;
    double rate=48000;
    unsigned channels=2,capacity=0;
};
#include "Audio32Editor.h"
juce::AudioProcessorEditor* Audio32Processor::createEditor() { return new Audio32Editor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Audio32Processor; }
