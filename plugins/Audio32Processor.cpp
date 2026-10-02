#include <juce_audio_processors/juce_audio_processors.h>
#include "effect_chain.hpp"
#include <array>

class Audio32Processor final : public juce::AudioProcessor {
public:
    Audio32Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
                                      .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
        parameters(*this,nullptr,"Audio32",layout()) {
        for (std::size_t i=0;i<ids.size();++i) values[i]=parameters.getRawParameterValue(ids[i]);
    }
    const juce::String getName() const override { return "Audio32"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override {
        const auto repeats=[](float feedback) { return feedback>0 ? std::max(1.0,std::log(0.001)/std::log(std::min(0.95F,feedback))) : 1.0; };
        const double reverb=values[15]->load()>0 ? 0.06*repeats(values[16]->load())+0.03 : 0;
        const double delay=values[18]->load()>0 ? values[20]->load()*repeats(values[19]->load()) : 0;
        return std::ceil(reverb+delay);
    }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout& buses) const override {
        const auto output=buses.getMainOutputChannelSet();
        return (output==juce::AudioChannelSet::mono() || output==juce::AudioChannelSet::stereo()) && output==buses.getMainInputChannelSet();
    }
    void prepareToPlay(double sampleRate,int maximumBlock) override {
        rate=sampleRate; channels=static_cast<unsigned>(getTotalNumOutputChannels());
        capacity=static_cast<unsigned>(std::max(1,maximumBlock));
        scratch.resize(capacity*channels);
        dsp.setFormat(rate,channels); update(); dsp.reset();
    }
    void releaseResources() override { dsp.reset(); }
    void reset() override { dsp.reset(); }
    void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override {
        juce::ScopedNoDenormals noDenormals;
        if (!capacity || buffer.getNumChannels()!=static_cast<int>(channels)) { buffer.clear(); return; }
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
    juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }
    void getStateInformation(juce::MemoryBlock& destination) override {
        if (auto xml=parameters.copyState().createXml()) copyXmlToBinary(*xml,destination);
    }
    void setStateInformation(const void* data,int size) override {
        if (auto xml=getXmlFromBinary(data,size))
            if (xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
private:
    static constexpr std::array<const char*,21> ids={"gain","ceiling","eqLow","eqMid","eqHigh","lowHz","midHz","highHz","midQ","threshold","ratio","makeup","attack","release","knee","reverbMix","reverbFeedback","damping","delayMix","delayFeedback","delaySeconds"};
    static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
        juce::AudioProcessorValueTreeState::ParameterLayout result;
        const auto add=[&](const char* id,const char* name,float low,float high,float initial) {
            result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(low,high),initial));
        };
        add("gain","Gain",0,4,1); add("ceiling","Output ceiling",0.01F,1,1);
        add("eqLow","Low gain",0.1F,4,1); add("eqMid","Mid gain",0.1F,4,1); add("eqHigh","High gain",0.1F,4,1);
        add("lowHz","Low frequency (Hz)",20,2000,150); add("midHz","Mid frequency (Hz)",20,20000,1000); add("highHz","High frequency (Hz)",1000,20000,6000);
        add("midQ","Mid Q",0.1F,10,0.707F); add("threshold","Threshold",0.001F,1,1);
        add("ratio","Ratio",1,100,1); add("makeup","Makeup gain",0,4,1); add("attack","Attack (ms)",0.01F,1000,10);
        add("release","Release (ms)",1,10000,100); add("knee","Knee (dB)",0,24,6);
        add("reverbMix","Reverb mix",0,1,0); add("reverbFeedback","Reverb feedback",0,0.95F,0.25F); add("damping","Reverb damping",0,0.99F,0.4F);
        add("delayMix","Delay mix",0,1,0); add("delayFeedback","Delay feedback",0,0.95F,0.25F); add("delaySeconds","Delay time (seconds)",0,2,0.25F);
        return result;
    }
    void update() {
        const auto v=[&](std::size_t i) { return values[i]->load(std::memory_order_relaxed); };
        audio32::EffectChainSettings settings;
        settings.gain=v(0); settings.ceiling=v(1);
        settings.equalizer={v(2),v(3),v(4),v(5),v(6),v(7),v(8)};
        settings.compressor={v(9),v(10),v(11),v(12),v(13),v(14)};
        settings.reverb={v(15),v(16),v(17)};
        settings.delay={v(18),v(19),static_cast<std::uint32_t>(v(20)*rate)};
        audio32::applyEffectChain(dsp,settings);
    }
    juce::AudioProcessorValueTreeState parameters;
    std::array<std::atomic<float>*,21> values{};
    audio32::DspEngine dsp;
    std::vector<float> scratch;
    double rate=48000;
    unsigned channels=2,capacity=0;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Audio32Processor; }
