#include <iomanip>
#include "dsp/digital.hpp"

#include "plugin.hpp"
#include "sid.h"
#include "voice_regs.h"
#include "filter_regs.h"

struct Sidofon : Module {
    enum ParamIds {
        PAL_NTSC_PARAM,
        OVERCLOCK_PARAM,
        PITCH_PARAM,
        PULSE_WIDTH_PARAM,
        WAVE_TRI_PARAM,
        WAVE_SAW_PARAM,
        WAVE_PULSE_PARAM,
        WAVE_NOISE_PARAM,
        GATE_PARAM,
        SYNC_PARAM,
        RING_MOD_PARAM,
        TEST_PARAM,
        ATTACK_PARAM,
        DECAY_PARAM,
        SUSTAIN_PARAM,
        RELEASE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        PULSE_WIDTH_INPUT,
        WAVE_TRI_INPUT,
        WAVE_SAW_INPUT,
        WAVE_PULSE_INPUT,
        WAVE_NOISE_INPUT,
        GATE_INPUT,
        SYNC_INPUT,
        RING_MOD_INPUT,
        TEST_INPUT,
        ATTACK_INPUT,
        DECAY_INPUT,
        SUSTAIN_INPUT,
        RELEASE_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUT_AUDIO,
        OUT_TRIGGER,
        NUM_OUTPUTS
    };
    enum LightIds {
        WAVE_TRI_LIGHT,
        WAVE_SAW_LIGHT,
        WAVE_PULSE_LIGHT,
        WAVE_NOISE_LIGHT,
        GATE_LIGHT,
        SYNC_LIGHT,
        RING_MOD_LIGHT,
        TEST_LIGHT,
        ATTACK_LIGHT,
        DECAY_LIGHT,
        SUSTAIN_LIGHT,
        RELEASE_LIGHT,
        NUM_LIGHTS
    };

    dsp::PulseGenerator pulseGen;
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

        configParam(PAL_NTSC_PARAM, 0.0f, 1.0f, 0.0f, "PAL/NTSC", "");
        configParam(OVERCLOCK_PARAM, 1.f, 8.f, 1.f, "Overclock", "x");
        configParam(PITCH_PARAM, -54.0, 54.0, 0.0, "Pitch", " Hz", std::pow(2.f, 1.f/12.f), dsp::FREQ_C4, 0.f);
        configParam(PULSE_WIDTH_PARAM, -1.0, 1.0, 0.0, "Pulse Width", " %", 0.f, 100.f);
        configParam(WAVE_TRI_PARAM, 0.0f, 1.0f, 0.0f, "Triangle", "");
        configParam(WAVE_SAW_PARAM, 0.0f, 1.0f, 0.0f, "Sawtooth", "");
        configParam(WAVE_PULSE_PARAM, 0.0f, 1.0f, 0.0f, "Pulse", "");
        configParam(WAVE_NOISE_PARAM, 0.0f, 1.0f, 0.0f, "Noise", "");
        configParam(GATE_PARAM, 0.0f, 1.0f, 0.0f, "Gate", "");
        configParam(SYNC_PARAM, 0.0f, 1.0f, 0.0f, "Sync", "");
        configParam(RING_MOD_PARAM, 0.0f, 1.0f, 0.0f, "Ring Mod", "");
        configParam(TEST_PARAM, 0.0f, 1.0f, 0.0f, "Test", "");
        configParam(ATTACK_PARAM, 0.0f, 1.0f, 0.0f, "Attack", " %", 0.f, 100.f);
        configParam(DECAY_PARAM, 0.0f, 1.0f, 0.0f, "Decay", " %", 0.f, 100.f);
        configParam(SUSTAIN_PARAM, 0.0f, 1.0f, 1.0f, "Sustain", " %", 0.f, 100.f);
        configParam(RELEASE_PARAM, 0.0f, 1.0f, 0.0f, "Release", " %", 0.f, 100.f);
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

    bool getSwitchValue(int inputId, int paramId) 
    {
        float val;
        if(inputs[inputId].isConnected()) {
            val = inputs[inputId].getVoltage();
        } else {
            val = params[paramId].getValue();
        }
        return val >= 1.0f;
    }

    uint8_t getADSRValue(int inputId, int paramId)
    {
        float val;
        if(inputs[inputId].isConnected()) {
            val = inputs[inputId].getVoltage() / 10.0f;
        } else {
            val = params[paramId].getValue() * 15.0f;
        }
        val = clamp(val * 15.f, 0.f, 15.f);
        return (uint8_t)val;
    }

    void process(const ProcessArgs& args) override {
        // reconfigure sid engine?
        if(cfgSampleRate != args.sampleRate) {
            cfgSampleRate = args.sampleRate;
            reconfigureSampleRate(args.sampleRate);

            // hack some voice0
            filterRegs.setVolume(FilterRegs::VOLUME_MAX);
            voiceRegs[0].setWaveform((uint8_t)VoiceRegs::WAVE_TRIANGLE);
            voiceRegs[0].setFreq(0x1cd6);
            voiceRegs[0].setSustain(VoiceRegs::SUSTAIN_MAX);
        }
    
        // update pitch
        float pitchKnob = params[PITCH_PARAM].getValue();
        float pitchCV = 12.f * inputs[PITCH_INPUT].getVoltage();
        float pitch = dsp::FREQ_C4 * std::pow(2.f, (pitchKnob + pitchCV) / 12.f);
        uint16_t pitchReg = freq2sidreg(pitch);
        voiceRegs[0].setFreq(pitchReg);

        // update gate
        bool gate = getSwitchValue(GATE_INPUT, GATE_PARAM);
        voiceRegs[0].setGate(gate);
        lights[GATE_LIGHT].setSmoothBrightness(gate, args.sampleTime);

        // update sync
        bool sync = getSwitchValue(SYNC_INPUT, SYNC_PARAM);
        voiceRegs[0].setSync(sync);
        lights[SYNC_LIGHT].setSmoothBrightness(sync, args.sampleTime);

        // update ringmod
        bool ringMod = getSwitchValue(RING_MOD_INPUT, RING_MOD_PARAM);
        voiceRegs[0].setRingMod(ringMod);
        lights[RING_MOD_LIGHT].setSmoothBrightness(ringMod, args.sampleTime);

        // update test
        bool test = getSwitchValue(TEST_INPUT, TEST_PARAM);
        voiceRegs[0].setTest(test);
        lights[TEST_LIGHT].setSmoothBrightness(test, args.sampleTime);

        // update ADSR
        voiceRegs[0].setAttack(getADSRValue(ATTACK_INPUT, ATTACK_PARAM));
        voiceRegs[0].setDecay(getADSRValue(DECAY_INPUT, DECAY_PARAM));
        voiceRegs[0].setSustain(getADSRValue(SUSTAIN_INPUT, SUSTAIN_PARAM));
        voiceRegs[0].setRelease(getADSRValue(RELEASE_INPUT, RELEASE_PARAM));
 
        // realize changed SID regs
        for(int i=0;i<VoiceRegs::NUM_VOICES;i++) {
            voiceRegs[i].realize(sid, i);
        }
        filterRegs.realize(sid);

        // emulate SID for some CPU clocks
        sid.clock(clockSteps);

        // retrieve SID audio sample and convert to voltage
        int16_t sample = sid.output();
        float audio = sample * 10.0f / 32768.0f;
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

        // params
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12, 26)), module, Sidofon::PITCH_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48, 26)), module, Sidofon::PULSE_WIDTH_PARAM));

        addParam(createParamCentered<CKSS>(mm2px(Vec(24, 24)), module, Sidofon::WAVE_TRI_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(28, 24)), module, Sidofon::WAVE_SAW_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(32, 24)), module, Sidofon::WAVE_PULSE_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(36, 24)), module, Sidofon::WAVE_NOISE_PARAM));

        addParam(createParamCentered<CKSS>(mm2px(Vec(24, 38)), module, Sidofon::GATE_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(28, 38)), module, Sidofon::SYNC_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(32, 38)), module, Sidofon::RING_MOD_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(36, 38)), module, Sidofon::TEST_PARAM));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(12, 48)), module, Sidofon::ATTACK_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(24, 48)), module, Sidofon::DECAY_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(36, 48)), module, Sidofon::SUSTAIN_PARAM));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(48, 48)), module, Sidofon::RELEASE_PARAM));

        addParam(createParamCentered<CKSS>(mm2px(Vec(106, 262)), module, Sidofon::PAL_NTSC_PARAM));

        // light
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17, 85)), module, Sidofon::WAVE_TRI_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(29, 85)), module, Sidofon::WAVE_SAW_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(41, 85)), module, Sidofon::WAVE_PULSE_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(53, 85)), module, Sidofon::WAVE_NOISE_LIGHT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17, 101)), module, Sidofon::GATE_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(29, 101)), module, Sidofon::SYNC_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(41, 101)), module, Sidofon::RING_MOD_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(53, 101)), module, Sidofon::TEST_LIGHT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17, 117)), module, Sidofon::ATTACK_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(29, 117)), module, Sidofon::DECAY_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(41, 117)), module, Sidofon::SUSTAIN_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(53, 117)), module, Sidofon::RELEASE_LIGHT));

        // inputs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12, 64)), module, Sidofon::PITCH_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36, 64)), module, Sidofon::PULSE_WIDTH_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12, 80)), module, Sidofon::WAVE_TRI_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24, 80)), module, Sidofon::WAVE_SAW_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36, 80)), module, Sidofon::WAVE_PULSE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48, 80)), module, Sidofon::WAVE_NOISE_INPUT));
        
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12, 96)), module, Sidofon::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24, 96)), module, Sidofon::SYNC_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36, 96)), module, Sidofon::RING_MOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48, 96)), module, Sidofon::TEST_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12, 112)), module, Sidofon::ATTACK_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24, 112)), module, Sidofon::DECAY_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36, 112)), module, Sidofon::SUSTAIN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48, 112)), module, Sidofon::RELEASE_INPUT));

        // outputs
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(72, 96)), module, Sidofon::OUT_AUDIO));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(72, 112)), module, Sidofon::OUT_TRIGGER));
    }
};


Model* modelSidofon = createModel<Sidofon, SidofonWidget>("sidofon");