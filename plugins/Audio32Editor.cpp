#include "Audio32Editor.h"
#include "Audio32Processor.cpp"
#include "effect_chain.hpp"
#include <functional>

Audio32Editor::Audio32Editor(Audio32Processor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(820, 600);
    setResizable(false, false);

    struct ParamDef { const char* id; const juce::String name; };
    const std::array<ParamDef, 21> defs {{
        {"gain", "Gain"}, {"ceiling", "Ceiling"},
        {"eqLow", "Low"}, {"eqMid", "Mid"}, {"eqHigh", "High"},
        {"lowHz", "Low Hz"}, {"midHz", "Mid Hz"}, {"highHz", "High Hz"}, {"midQ", "Q"},
        {"threshold", "Threshold"}, {"ratio", "Ratio"}, {"makeup", "Makeup"},
        {"attack", "Attack"}, {"release", "Release"}, {"knee", "Knee"},
        {"reverbMix", "Mix"}, {"reverbFeedback", "Feedback"}, {"damping", "Damping"},
        {"delayMix", "Mix"}, {"delayFeedback", "Feedback"}, {"delaySeconds", "Time"}
    }};

    auto addControl = [&](int idx, juce::Component* group) {
        auto slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        slider->setName(defs[idx].name);
        auto label = std::make_unique<juce::Label>();
        label->setText(defs[idx].name, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);

        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, defs[idx].id, *slider));

        group->addAndMakeVisible(slider.get());
        group->addAndMakeVisible(label.get());
        sliders.push_back(std::move(slider));
        labels.push_back(std::move(label));
    };

    addAndMakeVisible(eqGroup);
    addAndMakeVisible(compGroup);
    addAndMakeVisible(reverbGroup);
    addAndMakeVisible(delayGroup);
    addAndMakeVisible(masterGroup);
    addAndMakeVisible(midiGroup);
    addAndMakeVisible(presetGroup);
    addAndMakeVisible(meterGroup);
    vuMeterL = std::make_unique<juce::ProgressBar>(meterLevel);
    vuMeterR = std::make_unique<juce::ProgressBar>(meterLevel);
    meterGroup.addAndMakeVisible(vuMeterL.get());
    meterGroup.addAndMakeVisible(vuMeterR.get());

    bypassButton = std::make_unique<juce::ToggleButton>("Bypass");
    bypassButton->setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(bypassButton.get());
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters, "bypass", *bypassButton);

    loadChainButton = std::make_unique<juce::TextButton>("Load .chain");
    saveChainButton = std::make_unique<juce::TextButton>("Save .chain");
    presetGroup.addAndMakeVisible(loadChainButton.get());
    presetGroup.addAndMakeVisible(saveChainButton.get());

    loadChainButton->onClick = [this]() {
        juce::FileChooser chooser("Load Audio32 Chain Preset", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.chain");
        if (chooser.browseForFileToOpen()) {
            auto file = chooser.getResult();
            auto text = file.loadFileAsString().toStdString();
            if (auto settings = audio32::deserializeEffectChain(text)) {
                processor.applyPresetSettings(*settings);
            }
        }
    };
    saveChainButton->onClick = [this]() {
        juce::FileChooser chooser("Save Audio32 Chain Preset", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "*.chain", true);
        if (chooser.browseForFileToSave(true)) {
            auto file = chooser.getResult().withFileExtension(".chain");
            audio32::EffectChainSettings settings;
            auto v = [&](const char* id){ return processor.parameters.getRawParameterValue(id)->load(std::memory_order_relaxed); };
            settings.gain = v("gain"); settings.ceiling = v("ceiling");
            settings.equalizer = {v("eqLow"), v("eqMid"), v("eqHigh"), v("lowHz"), v("midHz"), v("highHz"), v("midQ")};
            settings.compressor = {v("threshold"), v("ratio"), v("makeup"), v("attack"), v("release"), v("knee")};
            settings.reverb = {v("reverbMix"), v("reverbFeedback"), v("damping")};
            settings.delay = {v("delayMix"), v("delayFeedback"), static_cast<std::uint32_t>(v("delaySeconds") * processor.rate)};
            auto str = audio32::serializeEffectChain(settings);
            file.replaceWithText(str);
        }
    };

    const std::vector<std::pair<int, juce::String>> learnList = {{0,"Gain"},{1,"Ceiling"},{2,"EQ Low"},{3,"EQ Mid"},{4,"EQ High"},{9,"Comp Thresh"},{10,"Comp Ratio"},{15,"Reverb Mix"},{18,"Delay Mix"}};
    for (size_t i = 0; i < learnList.size(); ++i) {
        auto btn = std::make_unique<juce::TextButton>("Learn CC: " + learnList[i].second);
        int target = learnList[i].first;
        btn->onClick = [this, target]() {
            processor.ccLearnTarget = target;
            processor.ccLearnActive = true;
        };
        midiGroup.addAndMakeVisible(btn.get());
        learnButtons.push_back(std::move(btn));
    }

    for (int i = 2; i <= 8; ++i) addControl(i, &eqGroup);
    for (int i = 9; i <= 14; ++i) addControl(i, &compGroup);
    for (int i = 15; i <= 17; ++i) addControl(i, &reverbGroup);
    for (int i = 18; i <= 20; ++i) addControl(i, &delayGroup);
    for (int i = 0; i <= 1; ++i) addControl(i, &masterGroup);
}

Audio32Editor::~Audio32Editor() {}

void Audio32Editor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("Audio32 Effects", getLocalBounds().removeFromTop(35), juce::Justification::centred, true);
    g.setFont(12.0f);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("EQ | Compressor | Reverb | Delay | Master | MIDI", getLocalBounds().removeFromTop(60).translated(0, 20), juce::Justification::centred, true);
}

void Audio32Editor::GroupComponent::GroupComponent(const juce::String& name)
{
    setText(name);
    setTextLabelPosition(juce::Label::centred);
}

void Audio32Editor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(40);

    if (bypassButton) {
        bypassButton->setBounds(area.removeFromTop(24).removeFromLeft(100));
    }

    auto masterArea = area.removeFromTop(95);
    masterGroup.setBounds(masterArea);
    auto m = masterGroup.getBounds().reduced(25, 25);
    if (sliders.size() > 1) { sliders[19]->setBounds(m.removeFromLeft(75)); labels[19]->setBounds(m.removeFromLeft(75).translated(0, 52)); }
    if (sliders.size() > 0) { sliders[18]->setBounds(m.removeFromLeft(75)); labels[18]->setBounds(m.removeFromLeft(75).translated(0, 52)); }

    auto row = area.removeFromTop(150);
    eqGroup.setBounds(row.removeFromLeft(400));
    auto e = eqGroup.getBounds().reduced(15, 25);
    for (int i = 0; i < 7 && (size_t)(2+i) < sliders.size(); ++i) { sliders[2+i]->setBounds(e.removeFromLeft(48)); labels[2+i]->setBounds(e.removeFromLeft(48).translated(0, 48)); }

    compGroup.setBounds(row.removeFromLeft(400));
    auto c = compGroup.getBounds().reduced(15, 25);
    for (int i = 0; i < 6 && (size_t)(9+i) < sliders.size(); ++i) { sliders[9+i]->setBounds(c.removeFromLeft(52)); labels[9+i]->setBounds(c.removeFromLeft(52).translated(0, 48)); }

    auto row2 = area.removeFromTop(150);
    reverbGroup.setBounds(row2.removeFromLeft(320));
    auto r = reverbGroup.getBounds().reduced(15, 25);
    for (int i = 0; i < 3 && (size_t)(15+i) < sliders.size(); ++i) { sliders[15+i]->setBounds(r.removeFromLeft(68)); labels[15+i]->setBounds(r.removeFromLeft(68).translated(0, 48)); }

    delayGroup.setBounds(row2.removeFromLeft(160));
    auto d = delayGroup.getBounds().reduced(15, 25);
    for (int i = 0; i < 3 && (size_t)(18+i) < sliders.size(); ++i) { sliders[18+i]->setBounds(d.removeFromLeft(48)); labels[18+i]->setBounds(d.removeFromLeft(48).translated(0, 48)); }

    midiGroup.setBounds(row2.removeFromLeft(180));
    auto ma = midiGroup.getBounds().reduced(8, 22);
    int y = 0;
    for (size_t i = 0; i < learnButtons.size(); ++i) {
        learnButtons[i]->setBounds(ma.removeFromTop(16).translated(0, y));
        y += 18;
    }

    auto presetArea = area.removeFromTop(60);
    presetGroup.setBounds(presetArea);
    auto pa = presetGroup.getBounds().reduced(15, 22);
    if (loadChainButton) loadChainButton->setBounds(pa.removeFromLeft(90));
    if (saveChainButton) saveChainButton->setBounds(pa.removeFromLeft(90));

    auto meterArea = area.removeFromTop(40);
    meterGroup.setBounds(meterArea);
    auto ma2 = meterGroup.getBounds().reduced(15, 12);
    if (vuMeterL) vuMeterL->setBounds(ma2.removeFromLeft(200));
    if (vuMeterR) vuMeterR->setBounds(ma2.removeFromLeft(200));
}
