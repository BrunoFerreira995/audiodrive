#include "audio_driver.hpp"
#include "audio_io.hpp"
#include "audio_unit.hpp"
#include "avfoundation.hpp"
#include "buffer.hpp"
#include "coreaudio.hpp"
#include "dsp.hpp"
#include "midi.hpp"
#include "mixer.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

void appendAscii(std::vector<std::uint8_t>& bytes, const char* text) {
    while (*text != '\0') {
        bytes.push_back(static_cast<std::uint8_t>(*text));
        ++text;
    }
}

void appendU16Le(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void appendU32Le(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

void writeBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::vector<std::uint8_t> makeWav() {
    std::vector<std::uint8_t> bytes;
    appendAscii(bytes, "RIFF");
    appendU32Le(bytes, 40);
    appendAscii(bytes, "WAVE");
    appendAscii(bytes, "fmt ");
    appendU32Le(bytes, 16);
    appendU16Le(bytes, 1);
    appendU16Le(bytes, 1);
    appendU32Le(bytes, 48000);
    appendU32Le(bytes, 96000);
    appendU16Le(bytes, 2);
    appendU16Le(bytes, 16);
    appendAscii(bytes, "data");
    appendU32Le(bytes, 4);
    appendU16Le(bytes, 0);
    appendU16Le(bytes, 32767);
    return bytes;
}

} // namespace

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
        audio32::DspEngine dsp;
        dsp.setFormat(48000.0, 1);
        dsp.setEqualizer({1.2F, 0.9F, 1.1F});
        dsp.setCompressor({0.25F, 4.0F, 1.0F});
        dsp.setReverb({0.25F, 0.5F});
        dsp.setDelay({0.5F, 0.25F, 1});
        std::vector<float> samples{0.5F, 0.0F, 0.0F, 0.0F};
        dsp.process(samples);
        assert(samples.size() == 4);
        assert(std::fabs(samples[0]) <= dsp.limiterCeiling());
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
        driver.setOutputDeviceUid("driver-test-device");
        assert(driver.outputDeviceUid() == "driver-test-device");
        assert(driver.initialize());
        assert(driver.write(std::vector<float>{0.0F, 0.0F}) == 2);
        assert(driver.underrunCount() == 0);
        assert(driver.lastBackendError() == 0);
        driver.setRecordingEnabled(true);
        driver.setLoopbackCaptureEnabled(true);
        assert(driver.isRecordingEnabled());
        assert(driver.isLoopbackCaptureEnabled());
        driver.setRecordingEnabled(false);
        driver.setLoopbackCaptureEnabled(false);
    }

    {
        audio32::CoreAudioBackend backend;
        backend.setOutputDeviceUid("test-device");
        assert(backend.outputDeviceUid() == "test-device");
        assert(!backend.initialize(48000.0, 2, {}));
        assert(!backend.start());
        assert(!backend.isRunning());
        assert(backend.underrunCount() == 0);
        assert(backend.lastError() == 0);
    }

    {
        const std::vector<std::int16_t> pcm16{-32768, 0, 32767};
        const auto floats = audio32::pcm16ToFloat(pcm16);
        assert(floats.size() == 3);
        assert(floats[0] == -1.0F);
        assert(floats[1] == 0.0F);
        assert(std::fabs(floats[2] - 0.999969F) < 0.00001F);

        const std::vector<std::uint8_t> pcm24{0x00, 0x00, 0x80, 0xFF, 0xFF, 0x7F};
        const auto floats24 = audio32::pcm24LittleEndianToFloat(pcm24);
        assert(floats24.size() == 2);
        assert(floats24[0] == -1.0F);
        assert(std::fabs(floats24[1] - 0.999999F) < 0.00001F);
    }

    {
        const auto path = std::filesystem::temp_directory_path() / "audio32_test.wav";
        writeBytes(path, makeWav());
        const auto decoded = audio32::decodeAudioFile(path);
        assert(decoded.has_value());
        assert(decoded->channels == 1);
        assert(decoded->sampleRate == 48000.0);
        assert(decoded->samples.size() == 2);
        assert(decoded->samples[0] == 0.0F);
        assert(decoded->samples[1] > 0.999F);
        std::filesystem::remove(path);
    }

    {
        audio32::MidiMapper mapper;
        audio32::DspEngine dsp;
        mapper.mapControl({7, audio32::MidiTarget::Gain, 0.0F, 2.0F});
        assert(mapper.applyControlChange(7, 127, dsp));
        assert(dsp.gain() == 2.0F);
        assert(!mapper.applyControlChange(8, 127, dsp));
    }

    {
        audio32::AudioUnitPluginDescriptor descriptor;
        assert(audio32::isValidAudioUnitDescriptor(descriptor));
        const auto identifier = audio32::makeAudioUnitComponentIdentifier(descriptor);
        assert(identifier.find(descriptor.bundleIdentifier) == 0);
    }

    {
        const auto integration = audio32::avFoundationIntegration();
#ifdef __APPLE__
        assert(integration.available);
        assert(integration.audioEngineSupported);
#else
        assert(!integration.available);
#endif
    }

    return 0;
}
