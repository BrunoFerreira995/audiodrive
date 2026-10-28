#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class Audio32Processor;

class Audio32Editor : public juce::AudioProcessorEditor
{
public:
    Audio32Editor(Audio32Processor&);
    ~Audio32Editor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Audio32Processor& processor;

    class GroupComponent : public juce::GroupComponent
    {
    public:
        GroupComponent(const juce::String& name);
    };

    GroupComponent eqGroup{"EQ"};
    GroupComponent compGroup{"Compressor"};
    GroupComponent reverbGroup{"Reverb"};
    GroupComponent delayGroup{"Delay"};
    GroupComponent masterGroup{"Master"};
    GroupComponent midiGroup{"MIDI CC Learn"};
    GroupComponent presetGroup{"Presets"};

    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::vector<std::unique_ptr<juce::TextButton>> learnButtons;
    std::unique_ptr<juce::ToggleButton> bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::TextButton> loadChainButton;
    std::unique_ptr<juce::TextButton> saveChainButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Audio32Editor)
};
