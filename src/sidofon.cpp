#include <iomanip>
#include "dsp/digital.hpp"

#include "plugin.hpp"
#include "sid.h"
#include "voice_regs.h"
#include "filter_regs.h"

struct Sidofon : Module {
    enum ParamIds {
        PITCH_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        GATE_INPUT,
        PITCH_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_AUDIO,
        OUT_TRIGGER,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    dsp::PulseGenerator pulseGen;
    dsp::SchmittTrigger gateDetect;
    bool gateOn;
    float counter;
    float period;

    reSID::SID sid;
    float cfgSampleRate;
    reSID::cycle_count clockSteps;
    static constexpr float clockHzPAL  =  985248.0f;
    static constexpr float clockHzNTSC = 1022727.0f;
    static constexpr float clockHz = clockHzNTSC;
    static constexpr float playHz = 50.0f;
    static constexpr float triggerTime = 1e-4f;
    uint16_t oldSidReg;

    VoiceRegs voiceRegs[VoiceRegs::NUM_VOICES];
    FilterRegs filterRegs;

    Sidofon() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configParam(PITCH_PARAM, -54.0, 54.0, 0.0, "Pitch", " Hz", std::pow(2.f, 1.f/12.f), dsp::FREQ_C4, 0.f);
    }

    inline uint16_t freq2sidreg(float freq)
    {
        return (uint16_t)freq * 16777216.0f / clockHz;
    }

    void reconfigureSampleRate(float sampleRate)
    {
        sid.reset();
        sid.set_sampling_parameters(clockHz, reSID::SAMPLE_FAST, sampleRate);
        
        // CPU clock steps between audio samples
        clockSteps = (reSID::cycle_count)roundf(clockHz / sampleRate);

        for(int i=0;i<VoiceRegs::NUM_VOICES;i++) {
            voiceRegs[i].reset();
        }
        filterRegs.reset();
    }

    void process(const ProcessArgs& args) override {
        // reconfigure sid engine?
        if(cfgSampleRate != args.sampleRate) {
            cfgSampleRate = args.sampleRate;
            reconfigureSampleRate(args.sampleRate);

            // hack some voice0
            filterRegs.setVolume(FilterRegs::VOLUME_MAX);
            voiceRegs[0].setWaveform(VoiceRegs::TRIANGLE);
            voiceRegs[0].setFreq(0x1cd6);
            voiceRegs[0].setSustain(VoiceRegs::SUSTAIN_MAX);
        }
    
        // update pitch
        float pitchKnob = params[PITCH_PARAM].getValue();
        float pitchCV = 12.f * inputs[PITCH_INPUT].getVoltage();
        float pitch = dsp::FREQ_C4 * std::pow(2.f, (pitchKnob + pitchCV) / 12.f);
        uint16_t pitchReg = freq2sidreg(pitch);
        voiceRegs[0].setFreq(pitchReg);

        // check gate
        float gateV = inputs[GATE_INPUT].getVoltage();
        bool gateHi = gateV >= 1.0f;
        bool detected = gateDetect.process(gateV);
        if(!gateOn && detected) {
            gateOn = true;
            voiceRegs[0].setGate(true);
        }
        else if(gateOn && !gateHi) {
            gateOn = false;
            voiceRegs[0].setGate(false);
        }

        // realize changed SID regs
        for(int i=0;i<VoiceRegs::NUM_VOICES;i++) {
            voiceRegs[i].realize(sid, i);
        }
        filterRegs.realize(sid);

        // emulate SID for some CPU clocks
        sid.clock(clockSteps);

        // retrieve SID audio sample and convert to voltage
        int16_t sample = sid.output();
        float audio = sample * 5.0f / 32768.0f;
        outputs[OUT_AUDIO].setVoltage(audio);

        // set trigger for vsync Hz
        period = args.sampleRate / playHz;
        if(counter > period) {
            pulseGen.trigger(triggerTime);
            counter -= period;
        }
        counter++;
        float triggerValue = pulseGen.process(args.sampleTime);
        outputs[OUT_TRIGGER].setVoltage(triggerValue * 10.f);
    }
};


struct SidofonWidget : ModuleWidget {
    SidofonWidget(Sidofon* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/sidofon.svg")));

        addParam(createParam<BefacoTinyKnob>(Vec(30, 40), module, Sidofon::PITCH_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 32.5)), module, Sidofon::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.457, 52.5)), module, Sidofon::PITCH_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 82.5)), module, Sidofon::OUT_AUDIO));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 100.5)), module, Sidofon::OUT_TRIGGER));
    }
};


Model* modelSidofon = createModel<Sidofon, SidofonWidget>("sidofon");