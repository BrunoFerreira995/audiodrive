#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
void check(OSStatus status,const char* operation) { if (status!=noErr) throw std::runtime_error(std::string(operation)+": "+std::to_string(status)); }
OSStatus input(void*,AudioUnitRenderActionFlags*,const AudioTimeStamp*,UInt32,UInt32 frames,AudioBufferList* buffers) {
    for (UInt32 b=0;b<buffers->mNumberBuffers;++b) {
        auto* samples=static_cast<float*>(buffers->mBuffers[b].mData);
        if (!samples) return kAudioUnitErr_InvalidPropertyValue;
        for (UInt32 i=0;i<frames*buffers->mBuffers[b].mNumberChannels;++i) samples[i]=0.1F;
    }
    return noErr;
}
int main(int argc,char** argv) {
    try {
        if (argc!=2) throw std::runtime_error("usage: au_host_tests plugin-binary-path");
        void* module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL); if (!module) throw std::runtime_error(dlerror());
        auto factory=reinterpret_cast<AudioComponentFactoryFunction>(dlsym(module,"Audio32_EffectsAUFactory"));
        if (!factory) throw std::runtime_error("AU factory missing");
        AudioComponentDescription description{};
        description.componentType=kAudioUnitType_Effect; description.componentSubType=0x41753332; description.componentManufacturer=0x41303332;
        auto component=AudioComponentRegister(&description,CFSTR("Audio32 host test"),256,factory);
        if (!component) throw std::runtime_error("component registration failed");
        AudioUnit unit=nullptr; check(AudioComponentInstanceNew(component,&unit),"instantiate");
        AudioStreamBasicDescription format{};
        format.mSampleRate=48000; format.mFormatID=kAudioFormatLinearPCM;
        format.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked;
        format.mBytesPerPacket=8; format.mFramesPerPacket=1; format.mBytesPerFrame=8; format.mChannelsPerFrame=2; format.mBitsPerChannel=32;
        check(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&format,sizeof(format)),"input format");
        check(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&format,sizeof(format)),"output format");
        AURenderCallbackStruct callback{input,nullptr};
        check(AudioUnitSetProperty(unit,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&callback,sizeof(callback)),"input callback");
        check(AudioUnitInitialize(unit),"initialize");
        std::vector<float> samples(256);
        AudioBufferList buffers{}; buffers.mNumberBuffers=1; buffers.mBuffers[0]={2,static_cast<UInt32>(samples.size()*sizeof(float)),samples.data()};
        for (unsigned block=0;block<20;++block) {
            AudioTimeStamp stamp{}; stamp.mSampleTime=block*128; stamp.mFlags=kAudioTimeStampSampleTimeValid;
            AudioUnitRenderActionFlags flags=0;
            check(AudioUnitRender(unit,&flags,&stamp,0,128,&buffers),"render");
            for (float sample:samples) if (!std::isfinite(sample) || std::fabs(sample-0.1F)>0.001F) throw std::runtime_error("neutral AU response");
        }
        CFPropertyListRef state=nullptr; UInt32 size=sizeof(state);
        check(AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,&size),"get state");
        check(AudioUnitSetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&state,sizeof(state)),"restore state");
        if (state) CFRelease(state);
        check(AudioUnitUninitialize(unit),"uninitialize"); check(AudioComponentInstanceDispose(unit),"dispose");
        // Keep module loaded while the registered factory remains visible to CoreAudio.
        std::cout<<"AudioUnit host render and state tests passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
