#include "native_audio.hpp"
#include "miniaudio.h"
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>
namespace audio32 {
struct NativeAudioBackend::Impl {
    ma_context context{};
    ma_device device{};
    bool contextReady=false, deviceReady=false;
    std::atomic<std::int32_t> error{0};
    Callback callback;
    static void render(ma_device* device, void* output, const void* input, ma_uint32 frames) noexcept {
        if (!device || !device->pUserData) return;
        if (frames == 0) return;
        auto& self=*static_cast<Impl*>(device->pUserData);
        try {
            const ma_uint32 channels = (device->type==ma_device_type_capture) ? device->capture.channels : device->playback.channels;
            if (channels == 0 || channels > 32) {
                self.error.store(MA_INVALID_ARGS);
                if (output) std::memset(output, 0, frames * channels * sizeof(float));
                return;
            }
            if (output && !self.callback) {
                std::memset(output, 0, frames * channels * sizeof(float));
                return;
            }
            self.callback(static_cast<float*>(output),static_cast<const float*>(input),frames, channels);
        }
        catch (...) {
            self.error.store(MA_ERROR);
            if (output) {
                const ma_uint32 channels = device->playback.channels ? device->playback.channels : device->capture.channels;
                if (channels > 0 && channels <= 32) std::memset(output,0,frames*channels*sizeof(float));
            }
        }
    }
    bool contextInit() {
        if (contextReady) return true;
#if defined(__APPLE__)
        const ma_backend backend=ma_backend_coreaudio;
#elif defined(_WIN32)
        const ma_backend backend=ma_backend_wasapi;
#elif defined(__linux__)
        const ma_backend backend=ma_backend_alsa;
#else
        error=MA_NO_BACKEND; return false;
#endif
#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
        const auto result=ma_context_init(&backend,1,nullptr,&context);
        error=result;
        contextReady=result==MA_SUCCESS;
#endif
        return contextReady;
    }
};
NativeAudioBackend::NativeAudioBackend():impl_(std::make_unique<Impl>()) {}
NativeAudioBackend::~NativeAudioBackend() {
    stop();
    if (impl_->deviceReady) ma_device_uninit(&impl_->device);
    if (impl_->contextReady) ma_context_uninit(&impl_->context);
}
std::vector<NativeDeviceInfo> NativeAudioBackend::devices() {
    if (!impl_->contextInit()) return {};
    ma_device_info *outputs=nullptr,*inputs=nullptr;
    ma_uint32 outputCount=0,inputCount=0;
    const auto result=ma_context_get_devices(&impl_->context,&outputs,&outputCount,&inputs,&inputCount);
    impl_->error=result;
    if (result!=MA_SUCCESS) return {};
    std::vector<NativeDeviceInfo> resultList;
    for (ma_uint32 i=0;i<outputCount;++i) {
#if defined(__APPLE__)
        const std::string id = outputs[i].id.coreaudio && outputs[i].id.coreaudio[0] ? std::string(outputs[i].id.coreaudio) : ("output:"+std::to_string(i));
#elif defined(_WIN32)
        const std::string id = outputs[i].id.wasapi && outputs[i].id.wasapi[0] ? std::string(outputs[i].id.wasapi) : ("wasapi:output:"+std::to_string(i));
#elif defined(__linux__)
        const std::string id = outputs[i].id.alsa ? ("alsa:" + std::string(outputs[i].name)) : ("alsa:output:"+std::to_string(i));
#else
        const std::string id="output:"+std::to_string(i);
#endif
        resultList.push_back({id,outputs[i].name,false,outputs[i].isDefault==MA_TRUE});
    }
    for (ma_uint32 i=0;i<inputCount;++i) {
#if defined(__APPLE__)
        const std::string id = inputs[i].id.coreaudio && inputs[i].id.coreaudio[0] ? std::string(inputs[i].id.coreaudio) : ("input:"+std::to_string(i));
#elif defined(_WIN32)
        const std::string id = inputs[i].id.wasapi && inputs[i].id.wasapi[0] ? std::string(inputs[i].id.wasapi) : ("wasapi:input:"+std::to_string(i));
#elif defined(__linux__)
        const std::string id = inputs[i].id.alsa ? ("alsa:" + std::string(inputs[i].name)) : ("alsa:input:"+std::to_string(i));
#else
        const std::string id="input:"+std::to_string(i);
#endif
        resultList.push_back({id,inputs[i].name,true,inputs[i].isDefault==MA_TRUE});
    }
    return resultList;
}
bool NativeAudioBackend::initialize(const NativeAudioConfig& config, Callback callback) {
    stop();
    if (impl_->deviceReady) { ma_device_uninit(&impl_->device); impl_->deviceReady=false; }
    if (!callback || (!config.playback && !config.capture) || config.channels<1 || config.channels>32 ||
        config.sampleRate<8000 || config.sampleRate>384000 || config.periodFrames<16 || config.periodFrames>8192) {
        impl_->error=MA_INVALID_ARGS; return false;
    }
    if (!impl_->contextInit()) return false;
    auto settings=ma_device_config_init(config.playback ? (config.capture?ma_device_type_duplex:ma_device_type_playback) : ma_device_type_capture);
    settings.sampleRate=config.sampleRate; settings.periodSizeInFrames=config.periodFrames;
    settings.playback.format=ma_format_f32; settings.playback.channels=config.channels;
    settings.capture.format=ma_format_f32; settings.capture.channels=config.channels;
    settings.dataCallback=Impl::render; settings.pUserData=impl_.get();
    ma_device_info *outputs=nullptr,*inputs=nullptr;
    ma_uint32 outputCount=0,inputCount=0;
    auto result=ma_context_get_devices(&impl_->context,&outputs,&outputCount,&inputs,&inputCount);
    if (result!=MA_SUCCESS) { impl_->error=result; return false; }
    ma_device_id outputId{},inputId{};
    const auto select=[&](const std::string& requested,ma_device_info* infos,ma_uint32 count,bool capture,ma_device_id& selected) {
        if (requested.empty()) return true;
        for (ma_uint32 i=0;i<count;++i) {
#if defined(__APPLE__)
            std::string id = infos[i].id.coreaudio && infos[i].id.coreaudio[0] ? std::string(infos[i].id.coreaudio) : (std::string(capture?"input:":"output:")+std::to_string(i));
#elif defined(_WIN32)
            std::string id = infos[i].id.wasapi && infos[i].id.wasapi[0] ? std::string(infos[i].id.wasapi) : (std::string(capture?"wasapi:input:":"wasapi:output:")+std::to_string(i));
#elif defined(__linux__)
            std::string id = infos[i].id.alsa ? std::string("alsa:")+infos[i].name : (std::string(capture?"alsa:input:":"alsa:output:")+std::to_string(i));
#else
            std::string id = std::string(capture?"input:":"output:")+std::to_string(i);
#endif
            if (id==requested) { selected=infos[i].id; return true; }
            // fallback by index if short form
            const std::string shortId = std::string(capture?"input:":"output:")+std::to_string(i);
            if (shortId==requested) { selected=infos[i].id; return true; }
        }
        return false;
    };
    if (!select(config.outputDeviceId,outputs,outputCount,false,outputId) || !select(config.inputDeviceId,inputs,inputCount,true,inputId)) {
        impl_->error=MA_NO_DEVICE; return false;
    }
    if (!config.outputDeviceId.empty()) settings.playback.pDeviceID=&outputId;
    if (!config.inputDeviceId.empty()) settings.capture.pDeviceID=&inputId;
    impl_->callback=std::move(callback);
    result=ma_device_init(&impl_->context,&settings,&impl_->device);
    impl_->error=result; impl_->deviceReady=result==MA_SUCCESS;
    return impl_->deviceReady;
}
bool NativeAudioBackend::start() {
    if (!impl_->deviceReady) { impl_->error=MA_INVALID_OPERATION; return false; }
    const auto result=ma_device_start(&impl_->device); impl_->error=result;
    return result==MA_SUCCESS;
}
void NativeAudioBackend::stop() noexcept {
    if (impl_->deviceReady) ma_device_stop(&impl_->device);
}
bool NativeAudioBackend::isRunning() const noexcept { return impl_->deviceReady && ma_device_is_started(&impl_->device); }
std::int32_t NativeAudioBackend::lastError() const noexcept { return impl_->error.load(); }
std::string NativeAudioBackend::backendName() const {
    return impl_->contextReady ? ma_get_backend_name(impl_->context.backend) : "uninitialized";
}
}
