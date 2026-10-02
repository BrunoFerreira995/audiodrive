#include "dsp.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>
#include <iostream>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
float response(float frequency, audio32::EqualizerSettings settings) {
    audio32::DspEngine dsp;
    dsp.setFormat(48000,1); dsp.setLimiterCeiling(16); dsp.setEqualizer(settings); dsp.reset();
    std::vector<float> signal(48000);
    for (std::size_t i=0;i<signal.size();++i) signal[i]=0.1F*std::sin(6.283185307179586*frequency*i/48000);
    dsp.process(signal);
    double energy=0;
    for (std::size_t i=24000;i<signal.size();++i) energy+=signal[i]*signal[i];
    return std::sqrt(energy/24000)*std::sqrt(2.0)/0.1;
}
int main() {
    require(std::fabs(response(1000,{})-1)<0.001,"neutral EQ");
    require(std::fabs(response(30,{2,1,1})-2)<0.02,"low shelf gain");
    require(std::fabs(response(1000,{1,2,1})-2)<0.01,"mid peak gain");
    require(std::fabs(response(18000,{1,1,2})-2)<0.02,"high shelf gain");
    {
        audio32::DspEngine dsp; dsp.setFormat(48000,2);
        dsp.setCompressor({0.25F,4,1,1,100,0}); dsp.reset();
        std::vector<float> x(48000*2);
        for (std::size_t i=0;i<x.size();i+=2) { x[i]=0.5F; x[i+1]=0.1F; }
        dsp.process(x);
        const float expected=0.25F*std::pow(2.0F,0.25F);
        require(std::fabs(x[x.size()-2]-expected)<0.001,"compressor dB ratio");
        require(std::fabs(x.back()/x[x.size()-2]-0.2F)<0.001,"linked stereo compression");
        require(x[0]>x[x.size()-2],"compressor attack");
    }
    {
        audio32::DspEngine dsp; dsp.setFormat(48000,2); dsp.setDelay({1,0.5F,100}); dsp.reset();
        std::vector<float> x(800,0); x[0]=1; dsp.process(x);
        require(std::fabs(x[200]-1)<0.001,"delay first echo");
        require(std::fabs(x[400]-0.5F)<0.001,"delay feedback");
        for (std::size_t i=1;i<x.size();i+=2) require(x[i]==0,"delay channel isolation");
        dsp.setReverb({0.2F,0.4F});
        std::vector<float> next(200,0); dsp.process(next);
        require(next[0]>0.01F,"unrelated settings preserve delay tail");
        dsp.reset(); std::vector<float> silence(1000,0); dsp.process(silence);
        for (float sample:silence) require(sample==0,"reset clears tails");
    }
    {
        audio32::DspEngine dsp; dsp.setFormat(48000,1); dsp.setReverb({1,0.8F,0.4F}); dsp.reset();
        std::vector<float> x(48000*5,0); x[0]=1; dsp.process(x);
        double early=0,late=0;
        for (std::size_t i=0;i<48000;++i) early+=x[i]*x[i];
        for (std::size_t i=x.size()-48000;i<x.size();++i) late+=x[i]*x[i];
        require(x[1]==0 && early>0.01,"reverb delayed diffuse tail");
        require(late<early*0.001,"reverb decays");
    }
    {
        audio32::DspEngine a,b;
        for (auto* dsp:{&a,&b}) {
            dsp->setEqualizer({1.5F,0.7F,1.2F}); dsp->setCompressor({0.3F,3,1});
            dsp->setReverb({0.4F,0.7F}); dsp->setDelay({0.3F,0.6F,137});
        }
        std::vector<float> x(12000);
        for (std::size_t i=0;i<x.size();++i) x[i]=0.5F*std::sin(i*0.1);
        auto y=x; a.process(x);
        for (std::size_t i=0;i<y.size();i+=74) b.process(std::span<float>(y).subspan(i,std::min<std::size_t>(74,y.size()-i)));
        require(x==y,"block partition invariance");
        b.setEqualizer({std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),0});
        b.setDelay({1,1, std::numeric_limits<unsigned>::max()}); b.setFormat(8000,2);
        y.assign(10000,0.1F); y[0]=std::numeric_limits<float>::quiet_NaN(); b.process(y);
        for (float value:y) require(std::isfinite(value) && std::fabs(value)<=1,"finite bounded output");
    }
    std::cout << "DSP tests passed\n";
}
