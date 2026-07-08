#include "audio_driver.hpp"
#include "buffer.hpp"
#include "dsp.hpp"
#include "mixer.hpp"

#include <cassert>
#include <cmath>
#include <vector>

int main() {
    {
        audio32::AudioBuffer buffer(16);
        const std::vector<float> input{1.0F, 2.0F, 3.0F, 4.0F};
        std::vector<float> output(4, 0.0F);

        assert(buffer.write(input) == input.size());
        assert(buffer.read(output) == output.size());
        assert(output == input);
    }

    {
        audio32::DspEngine dsp;
        dsp.setGain(2.0F);
        dsp.setLimiterCeiling(0.75F);
        std::vector<float> samples{-1.0F, -0.25F, 0.25F, 1.0F};
        dsp.process(samples);

        assert(samples[0] == -0.75F);
        assert(samples[1] == -0.5F);
        assert(samples[2] == 0.5F);
        assert(samples[3] == 0.75F);
    }

    {
        audio32::Mixer mixer;
        mixer.setOutputChannels(2);
        const std::vector<float> mono{0.1F, 0.2F};
        std::vector<float> stereo(4, 0.0F);
        mixer.mixMonoToOutput(mono, stereo);

        assert(stereo[0] == 0.1F);
        assert(stereo[1] == 0.1F);
        assert(stereo[2] == 0.2F);
        assert(stereo[3] == 0.2F);
    }

    {
        audio32::AudioDriver driver;
        driver.setSampleRate(48000.0);
        driver.setChannels(2);
        assert(driver.initialize());
        assert(driver.write(std::vector<float>{0.0F, 0.0F}) == 2);
    }

    return 0;
}
