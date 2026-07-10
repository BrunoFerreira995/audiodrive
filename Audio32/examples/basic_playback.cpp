#include "audio_driver.hpp"

#include <cmath>
#include <chrono>
#include <iostream>
#include <numbers>
#include <thread>
#include <vector>

int main() {
    audio32::AudioDriver driver;
    driver.setSampleRate(48000.0);
    driver.setChannels(2);
    driver.dsp().setGain(0.25F);

    if (!driver.initialize()) {
        std::cerr << "failed to initialize Audio32\n";
        return 1;
    }

    constexpr auto frames = 48000;
    std::vector<float> samples(frames * 2);
    for (int frame = 0; frame < frames; ++frame) {
        const auto t = static_cast<float>(frame) / 48000.0F;
        const auto sample = std::sin(2.0F * std::numbers::pi_v<float> * 440.0F * t);
        samples[frame * 2] = sample;
        samples[frame * 2 + 1] = sample;
    }

    const auto written = driver.write(samples);
    std::cout << "queued " << written << " float samples\n";

    if (!driver.start()) {
        std::cerr << "failed to start Audio32, backend error " << driver.lastBackendError() << "\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    driver.stop();
    return 0;
}
