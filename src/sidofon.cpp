#include <iomanip>
#include "dsp/digital.hpp"

#include "plugin.hpp"
#include "sid.h"
#include "voice_regs.h"
#include "filter_regs.h"

struct Sidofon : Module {
    enum ParamIds {
        // Voices
        ENUMS(PITCH_PARAM,3),
        ENUMS(PULSE_WIDTH_PARAM,3),
        ENUMS(WAVE_TRI_PARAM,3),
        ENUMS(WAVE_SAW_PARAM,3),
        ENUMS(WAVE_PULSE_PARAM,3),
        ENUMS(WAVE_NOISE_PARAM,3),
        ENUMS(GATE_PARAM,3),
        ENUMS(SYNC_PARAM,3),
        ENUMS(RING_MOD_PARAM,3),
        ENUMS(TEST_PARAM,3),
        ENUMS(ATTACK_PARAM,3),
        ENUMS(DECAY_PARAM,3),
        ENUMS(SUSTAIN_PARAM,3),
        ENUMS(RELEASE_PARAM,3),
        // Filter
        FILTER_VOICE1_PARAM,
        FILTER_VOICE2_PARAM,
        FILTER_VOICE3_PARAM,
        FILTER_AUX_PARAM,
        FILTER_LO_PASS_PARAM,
        FILTER_BAND_PASS_PARAM,
        FILTER_HI_PASS_PARAM,
        VOICE3OFF_PARAM,
        FILTER_CUTOFF_PARAM,
        FILTER_RESONANCE_PARAM,
        VOLUME_PARAM,
        // Clock
        PAL_NTSC_PARAM,
        OVERCLOCK_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        // Voices
        ENUMS(PITCH_INPUT,3),
        ENUMS(PULSE_WIDTH_INPUT,3),
        ENUMS(WAVE_TRI_INPUT,3),
        ENUMS(WAVE_SAW_INPUT,3),
        ENUMS(WAVE_PULSE_INPUT,3),
        ENUMS(WAVE_NOISE_INPUT,3),
        ENUMS(GATE_INPUT,3),
        ENUMS(SYNC_INPUT,3),
        ENUMS(RING_MOD_INPUT,3),
        ENUMS(TEST_INPUT,3),
        ENUMS(ATTACK_INPUT,3),
        ENUMS(DECAY_INPUT,3),
        ENUMS(SUSTAIN_INPUT,3),
        ENUMS(RELEASE_INPUT,3),
        // Filter
        FILTER_VOICE1_INPUT,
        FILTER_VOICE2_INPUT,
        FILTER_VOICE3_INPUT,
        FILTER_AUX_INPUT,
        FILTER_LO_PASS_INPUT,
        FILTER_BAND_PASS_INPUT,
        FILTER_HI_PASS_INPUT,
        VOICE3OFF_INPUT,
        FILTER_CUTOFF_INPUT,
        FILTER_RESONANCE_INPUT,
        VOLUME_INPUT,
        // Main
        AUX_INPUT,
        CLOCK_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        CLOCK_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        // Voices
        ENUMS(WAVE_TRI_LIGHT,3),
        ENUMS(WAVE_SAW_LIGHT,3),
        ENUMS(WAVE_PULSE_LIGHT,3),
        ENUMS(WAVE_NOISE_LIGHT,3),
        ENUMS(GATE_LIGHT,3),
        ENUMS(SYNC_LIGHT,3),
        ENUMS(RING_MOD_LIGHT,3),
        ENUMS(TEST_LIGHT,3),
        ENUMS(ATTACK_LIGHT,3),
        ENUMS(DECAY_LIGHT,3),
        ENUMS(SUSTAIN_LIGHT,3),
        ENUMS(RELEASE_LIGHT,3),
        // Filter
        FILTER_VOICE1_LIGHT,
        FILTER_VOICE2_LIGHT,
        FILTER_VOICE3_LIGHT,
        FILTER_AUX_LIGHT,
        FILTER_LO_PASS_LIGHT,
        FILTER_BAND_PASS_LIGHT,
        FILTER_HI_PASS_LIGHT,
        VOICE3OFF_LIGHT,
        FILTER_CUTOFF_LIGHT,
        FILTER_RESONANCE_LIGHT,
        VOLUME_LIGHT,
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

        // voices
        for(int i=0;i<3;i++) {
            configParam(PITCH_PARAM + i, -54.0, 54.0, 0.0, "Pitch", " Hz", std::pow(2.f, 1.f/12.f), dsp::FREQ_C4, 0.f);
            configParam(PULSE_WIDTH_PARAM + i, -1.0, 1.0, 0.0, "Pulse Width", " %", 0.f, 100.f);
            configParam(WAVE_TRI_PARAM + i, 0.0f, 1.0f, 0.0f, "Triangle", "");
            configParam(WAVE_SAW_PARAM + i, 0.0f, 1.0f, 0.0f, "Sawtooth", "");
            configParam(WAVE_PULSE_PARAM + i, 0.0f, 1.0f, 0.0f, "Pulse", "");
            configParam(WAVE_NOISE_PARAM + i, 0.0f, 1.0f, 0.0f, "Noise", "");
            configParam(GATE_PARAM + i, 0.0f, 1.0f, 0.0f, "Gate", "");
            configParam(SYNC_PARAM + i, 0.0f, 1.0f, 0.0f, "Sync", "");
            configParam(RING_MOD_PARAM + i, 0.0f, 1.0f, 0.0f, "Ring Mod", "");
            configParam(TEST_PARAM + i, 0.0f, 1.0f, 0.0f, "Test", "");
            configParam(ATTACK_PARAM + i, 0.0f, 1.0f, 0.0f, "Attack");
            configParam(DECAY_PARAM + i, 0.0f, 1.0f, 0.0f, "Decay");
            configParam(SUSTAIN_PARAM + i, 0.0f, 1.0f, 1.0f, "Sustain");
            configParam(RELEASE_PARAM + i, 0.0f, 1.0f, 0.0f, "Release");
        }

        // filter
        configParam(FILTER_VOICE1_PARAM, 0.0f, 1.0f, 0.0f, "Filter Voice 1");
        configParam(FILTER_VOICE2_PARAM, 0.0f, 1.0f, 0.0f, "Filter Voice 2");
        configParam(FILTER_VOICE3_PARAM, 0.0f, 1.0f, 0.0f, "Filter Voice 3");
        configParam(FILTER_AUX_PARAM, 0.0f, 1.0f, 0.0f, "Filter Aux");
        configParam(FILTER_LO_PASS_PARAM, 0.0f, 1.0f, 0.0f, "Low Pass Filter");
        configParam(FILTER_BAND_PASS_PARAM, 0.0f, 1.0f, 0.0f, "Band Pass Filter");
        configParam(FILTER_HI_PASS_PARAM, 0.0f, 1.0f, 0.0f, "High Pass Filter");
        configParam(VOICE3OFF_PARAM, 0.0f, 1.0f, 0.0f, "Voice 3 Off");
        configParam(FILTER_CUTOFF_PARAM, 0.0f, 1.0f, 0.5f, "Filter Cut Off Freq");
        configParam(FILTER_RESONANCE_PARAM, 0.0f, 1.0f, 0.0f, "Filter Resonance");
        configParam(VOLUME_PARAM, 0.0f, 1.0f, 1.0f, "Master Volume");

        // clock
        configParam(PAL_NTSC_PARAM, 0.0f, 1.0f, 0.0f, "PAL/NTSC", "");
        configParam(OVERCLOCK_PARAM, 1.f, 8.f, 1.f, "Overclock", "x");
      }

    inline uint16_t freq2sidreg(float freq)
    {
        return (uint16_t)(freq * 16777216.0f / clockHz);
    }

    void reset(float sampleRate)
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
        float val = params[paramId].getValue();
        if(inputs[inputId].isConnected()) {
            val += inputs[inputId].getVoltage();
        }
        return val >= 1.0f;
    }

    uint8_t getByteValue(int inputId, int paramId, int max)
    {
        float val = params[paramId].getValue();
        if(inputs[inputId].isConnected()) {
            val += inputs[inputId].getVoltage() / 10.0f;
        }
        val = clamp(val * (float)max, 0.f, (float)max);
        return (uint8_t)val;
    }

    uint16_t getLoHiValue(int inputId, int paramId, int max)
    {
        float val = params[paramId].getValue();
        if(inputs[inputId].isConnected()) {
            val += inputs[inputId].getVoltage() / 10.0f;
        }
        val = clamp(val * (float)max, 0.f, (float)max);
        return (uint16_t)val;
    }

    void updateVoice(int voiceNo)
    {
        VoiceRegs &regs = voiceRegs[voiceNo];

        // update pitch
        float pitchKnob = params[PITCH_PARAM + voiceNo].getValue();
        float pitchCV = 12.f * inputs[PITCH_INPUT + voiceNo].getVoltage();
        float pitch = dsp::FREQ_C4 * std::pow(2.f, (pitchKnob + pitchCV) / 12.f);
        uint16_t pitchReg = freq2sidreg(pitch);
        regs.setFreq(pitchReg);

        // update pulse width
        float pwKnob = params[PULSE_WIDTH_PARAM + voiceNo].getValue();
        float pwInput = inputs[PULSE_WIDTH_INPUT + voiceNo].getVoltage();
        float pw = (clamp(pwKnob + pwInput / 5.0f, -1.0f, 1.0f) + 1.0f) / 2.0f;
        uint16_t pwVal = (uint16_t)(pw * VoiceRegs::PULSE_WIDTH_MAX);
        regs.setPulseWidth(pwVal);

        // update waveform
        uint8_t waveform = 0;
        bool tri = getSwitchValue(WAVE_TRI_INPUT + voiceNo, WAVE_TRI_PARAM + voiceNo);
        bool saw = getSwitchValue(WAVE_SAW_INPUT + voiceNo, WAVE_SAW_PARAM + voiceNo);
        bool pulse = getSwitchValue(WAVE_PULSE_INPUT + voiceNo, WAVE_PULSE_PARAM + voiceNo);
        bool noise = getSwitchValue(WAVE_NOISE_INPUT + voiceNo, WAVE_NOISE_PARAM + voiceNo);
        if(tri) waveform |= VoiceRegs::WAVE_TRIANGLE;
        if(saw) waveform |= VoiceRegs::WAVE_SAWTOOTH;
        if(pulse) waveform |= VoiceRegs::WAVE_RECTANGLE;
        if(noise) waveform |= VoiceRegs::WAVE_NOISE;
        regs.setWaveform(waveform);

        // update gate
        bool gate = getSwitchValue(GATE_INPUT + voiceNo, GATE_PARAM + voiceNo);
        regs.setGate(gate);

        // update sync
        bool sync = getSwitchValue(SYNC_INPUT + voiceNo, SYNC_PARAM + voiceNo);
        regs.setSync(sync);

        // update ringmod
        bool ringMod = getSwitchValue(RING_MOD_INPUT + voiceNo, RING_MOD_PARAM + voiceNo);
        regs.setRingMod(ringMod);

        // update test
        bool test = getSwitchValue(TEST_INPUT + voiceNo, TEST_PARAM + voiceNo);
        regs.setTest(test);

        // update ADSR
        regs.setAttack(getByteValue(ATTACK_INPUT + voiceNo, ATTACK_PARAM + voiceNo, VoiceRegs::ATTACK_MAX));
        regs.setDecay(getByteValue(DECAY_INPUT + voiceNo, DECAY_PARAM + voiceNo, VoiceRegs::DECAY_MAX));
        regs.setSustain(getByteValue(SUSTAIN_INPUT + voiceNo, SUSTAIN_PARAM + voiceNo, VoiceRegs::SUSTAIN_MAX));
        regs.setRelease(getByteValue(RELEASE_INPUT + voiceNo, RELEASE_PARAM + voiceNo, VoiceRegs::RELEASE_MAX));
    }

    void updateVoiceLights(int voiceNo, float sampleTime)
    {
        VoiceRegs &regs = voiceRegs[voiceNo];

        uint8_t waveform = regs.getWaveform();
        bool tri = (waveform & VoiceRegs::WAVE_TRIANGLE) == VoiceRegs::WAVE_TRIANGLE;
        bool saw = (waveform & VoiceRegs::WAVE_SAWTOOTH) == VoiceRegs::WAVE_SAWTOOTH;
        bool pulse = (waveform & VoiceRegs::WAVE_RECTANGLE) == VoiceRegs::WAVE_RECTANGLE;
        bool noise = (waveform & VoiceRegs::WAVE_NOISE) == VoiceRegs::WAVE_NOISE;
        lights[WAVE_TRI_LIGHT + voiceNo].setSmoothBrightness(tri, sampleTime);
        lights[WAVE_SAW_LIGHT + voiceNo].setSmoothBrightness(saw, sampleTime);
        lights[WAVE_PULSE_LIGHT + voiceNo].setSmoothBrightness(pulse, sampleTime);
        lights[WAVE_NOISE_LIGHT + voiceNo].setSmoothBrightness(noise, sampleTime);

        lights[GATE_LIGHT + voiceNo].setSmoothBrightness(regs.getGate(), sampleTime);
        lights[SYNC_LIGHT + voiceNo].setSmoothBrightness(regs.getSync(), sampleTime);
        lights[RING_MOD_LIGHT + voiceNo].setSmoothBrightness(regs.getRingMod(), sampleTime);
        lights[TEST_LIGHT + voiceNo].setSmoothBrightness(regs.getTest(), sampleTime);

        lights[ATTACK_LIGHT + voiceNo].setSmoothBrightness((float)regs.getAttack() / 15.0f, sampleTime);
        lights[DECAY_LIGHT + voiceNo].setSmoothBrightness((float)regs.getDecay() / 15.0f, sampleTime);
        lights[SUSTAIN_LIGHT + voiceNo].setSmoothBrightness((float)regs.getSustain() / 15.0f, sampleTime);
        lights[RELEASE_LIGHT + voiceNo].setSmoothBrightness((float)regs.getRelease() / 15.0f, sampleTime);
    }

    void updateFilter()
    {
        bool voice1 = getSwitchValue(FILTER_VOICE1_INPUT, FILTER_VOICE1_PARAM);
        filterRegs.setFilterVoice(0, voice1);
        bool voice2 = getSwitchValue(FILTER_VOICE2_INPUT, FILTER_VOICE2_PARAM);
        filterRegs.setFilterVoice(1, voice2);
        bool voice3 = getSwitchValue(FILTER_VOICE3_INPUT, FILTER_VOICE3_PARAM);
        filterRegs.setFilterVoice(2, voice3);
        bool ext = getSwitchValue(FILTER_AUX_INPUT, FILTER_AUX_PARAM); 
        filterRegs.setFilterExt(ext);

        bool lp = getSwitchValue(FILTER_LO_PASS_INPUT, FILTER_LO_PASS_PARAM);
        bool bp = getSwitchValue(FILTER_BAND_PASS_INPUT, FILTER_BAND_PASS_PARAM);
        bool hp = getSwitchValue(FILTER_HI_PASS_INPUT, FILTER_HI_PASS_PARAM);
        uint8_t mode = 0;
        if(lp) mode |= FilterRegs::MODE_LP;
        if(bp) mode |= FilterRegs::MODE_BP;
        if(hp) mode |= FilterRegs::MODE_HP;
        filterRegs.setMode(mode);

        bool v3off = getSwitchValue(VOICE3OFF_INPUT, VOICE3OFF_PARAM);
        filterRegs.setVoice3Off(v3off);

        uint16_t cutoff = getLoHiValue(FILTER_CUTOFF_INPUT, FILTER_CUTOFF_PARAM, FilterRegs::CUTOFF_MAX);
        filterRegs.setCutOff(cutoff);
        
        uint8_t  res = getByteValue(FILTER_RESONANCE_INPUT, FILTER_RESONANCE_PARAM, FilterRegs::RESONANCE_MAX);
        filterRegs.setResonance(res);
        
        uint8_t  vol = getByteValue(VOLUME_INPUT, VOLUME_PARAM, FilterRegs::VOLUME_MAX);
        filterRegs.setVolume(vol);
    }

    void updateFilterLights(float sampleTime)
    {
        lights[FILTER_VOICE1_LIGHT].setSmoothBrightness(filterRegs.getFilterVoice(0), sampleTime);
        lights[FILTER_VOICE2_LIGHT].setSmoothBrightness(filterRegs.getFilterVoice(1), sampleTime);
        lights[FILTER_VOICE3_LIGHT].setSmoothBrightness(filterRegs.getFilterVoice(2), sampleTime);
        lights[FILTER_AUX_LIGHT].setSmoothBrightness(filterRegs.getFilterExt(), sampleTime);

        uint8_t mode = filterRegs.getMode();
        bool lp = (mode & FilterRegs::MODE_LP) == FilterRegs::MODE_LP;
        bool bp = (mode & FilterRegs::MODE_BP) == FilterRegs::MODE_BP;
        bool hp = (mode & FilterRegs::MODE_HP) == FilterRegs::MODE_HP;
        lights[FILTER_LO_PASS_LIGHT].setSmoothBrightness(lp, sampleTime);
        lights[FILTER_BAND_PASS_LIGHT].setSmoothBrightness(bp, sampleTime);
        lights[FILTER_HI_PASS_LIGHT].setSmoothBrightness(hp, sampleTime);

        lights[VOICE3OFF_LIGHT].setSmoothBrightness(filterRegs.getVoice3Off(), sampleTime);

        float cutoff = (float)filterRegs.getCutOff() / (float)FilterRegs::CUTOFF_MAX;
        lights[FILTER_CUTOFF_LIGHT].setSmoothBrightness(cutoff, sampleTime);

        float resonance = (float)filterRegs.getResonance() / (float)FilterRegs::RESONANCE_MAX;
        lights[FILTER_RESONANCE_LIGHT].setSmoothBrightness(resonance, sampleTime);

        float volume = (float)filterRegs.getVolume() / (float)FilterRegs::VOLUME_MAX;
        lights[VOLUME_LIGHT].setSmoothBrightness(volume, sampleTime);
    }

    void process(const ProcessArgs& args) override {
        // reconfigure sid engine?
        if(cfgSampleRate != args.sampleRate) {
            cfgSampleRate = args.sampleRate;
            reset(args.sampleRate);
        }
    
        // uptdate voices
        for(int i=0;i<VoiceRegs::NUM_VOICES;i++) {
            updateVoice(i);
        }
        // update filter
        updateFilter();

        // realize changed SID regs
        for(int i=0;i<VoiceRegs::NUM_VOICES;i++) {
            voiceRegs[i].realize(sid, i);
            updateVoiceLights(i,args.sampleTime);
        }
        filterRegs.realize(sid);
        updateFilterLights(args.sampleTime);

        // emulate SID for some CPU clocks
        sid.clock(clockSteps);

        // retrieve SID audio sample and convert to voltage
        int16_t sample = sid.output();
        float audio = sample * 10.0f / 32768.0f;
        outputs[AUDIO_OUTPUT].setVoltage(audio);

        // set trigger for vsync Hz
        period = args.sampleRate / playHz;
        if(counter > period) {
            pulseGen.trigger(triggerTime);
            counter -= period;
        }
        counter++;
        float triggerValue = pulseGen.process(args.sampleTime);
        outputs[CLOCK_OUTPUT].setVoltage(triggerValue * 10.f);
    }
};


struct SidofonWidget : ModuleWidget {
    SidofonWidget(Sidofon* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/sidofon.svg")));

        // ----- Voices -----
        for(int i=0;i<3;i++) {
            addVoice(i, i * 56);
        }

        // ----- Filter -----
        // params
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(180, 26)), module, Sidofon::FILTER_CUTOFF_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(198, 26)), module, Sidofon::FILTER_RESONANCE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(216, 26)), module, Sidofon::VOLUME_PARAM));

        addParam(createParamCentered<CKSS>(mm2px(Vec(178, 46)), module, Sidofon::FILTER_VOICE1_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(182, 46)), module, Sidofon::FILTER_VOICE2_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(186, 46)), module, Sidofon::FILTER_VOICE3_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(194, 46)), module, Sidofon::FILTER_AUX_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(202, 46)), module, Sidofon::VOICE3OFF_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(210, 46)), module, Sidofon::FILTER_LO_PASS_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(214, 46)), module, Sidofon::FILTER_BAND_PASS_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(218, 46)), module, Sidofon::FILTER_HI_PASS_PARAM));

        // light
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(185, 69)), module, Sidofon::FILTER_VOICE1_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(197, 69)), module, Sidofon::FILTER_VOICE2_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(209, 69)), module, Sidofon::FILTER_VOICE3_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(221, 69)), module, Sidofon::FILTER_AUX_LIGHT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(185, 85)), module, Sidofon::FILTER_LO_PASS_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(197, 85)), module, Sidofon::FILTER_BAND_PASS_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(209, 85)), module, Sidofon::FILTER_HI_PASS_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(221, 85)), module, Sidofon::VOICE3OFF_LIGHT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(185, 101)), module, Sidofon::FILTER_CUTOFF_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(197, 101)), module, Sidofon::FILTER_RESONANCE_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(221, 101)), module, Sidofon::VOLUME_LIGHT));

        // inputs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(180, 64)), module, Sidofon::FILTER_VOICE1_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(192, 64)), module, Sidofon::FILTER_VOICE2_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(204, 64)), module, Sidofon::FILTER_VOICE3_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(216, 64)), module, Sidofon::FILTER_AUX_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(180, 80)), module, Sidofon::FILTER_LO_PASS_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(192, 80)), module, Sidofon::FILTER_BAND_PASS_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(204, 80)), module, Sidofon::FILTER_HI_PASS_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(216, 80)), module, Sidofon::VOICE3OFF_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(180, 96)), module, Sidofon::FILTER_CUTOFF_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(192, 96)), module, Sidofon::FILTER_RESONANCE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(216, 96)), module, Sidofon::VOLUME_INPUT));

        // ----- Main -----
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(180, 112)), module, Sidofon::AUX_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(192, 112)), module, Sidofon::CLOCK_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(204, 112)), module, Sidofon::CLOCK_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(216, 112)), module, Sidofon::AUDIO_OUTPUT));
    }

    void addVoice(int voiceNo, int offset) {
        // params
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12 + offset, 26)), module, Sidofon::PITCH_PARAM + voiceNo));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48 + offset, 26)), module, Sidofon::PULSE_WIDTH_PARAM + voiceNo));

        addParam(createParamCentered<CKSS>(mm2px(Vec(24 + offset, 24)), module, Sidofon::WAVE_TRI_PARAM + voiceNo));
        addParam(createParamCentered<CKSS>(mm2px(Vec(28 + offset, 24)), module, Sidofon::WAVE_SAW_PARAM + voiceNo));
        addParam(createParamCentered<CKSS>(mm2px(Vec(32 + offset, 24)), module, Sidofon::WAVE_PULSE_PARAM + voiceNo));
        addParam(createParamCentered<CKSS>(mm2px(Vec(36 + offset, 24)), module, Sidofon::WAVE_NOISE_PARAM + voiceNo));

        addParam(createParamCentered<CKSS>(mm2px(Vec(24 + offset, 38)), module, Sidofon::GATE_PARAM + voiceNo));
        addParam(createParamCentered<CKSS>(mm2px(Vec(28 + offset, 38)), module, Sidofon::SYNC_PARAM + voiceNo));
        addParam(createParamCentered<CKSS>(mm2px(Vec(32 + offset, 38)), module, Sidofon::RING_MOD_PARAM + voiceNo));
        addParam(createParamCentered<CKSS>(mm2px(Vec(36 + offset, 38)), module, Sidofon::TEST_PARAM + voiceNo));

        addParam(createParamCentered<Trimpot>(mm2px(Vec(12 + offset, 48)), module, Sidofon::ATTACK_PARAM + voiceNo));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(24 + offset, 48)), module, Sidofon::DECAY_PARAM + voiceNo));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(36 + offset, 48)), module, Sidofon::SUSTAIN_PARAM + voiceNo));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(48 + offset, 48)), module, Sidofon::RELEASE_PARAM + voiceNo));

        // light
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17 + offset, 85)), module, Sidofon::WAVE_TRI_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(29 + offset, 85)), module, Sidofon::WAVE_SAW_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(41 + offset, 85)), module, Sidofon::WAVE_PULSE_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(53 + offset, 85)), module, Sidofon::WAVE_NOISE_LIGHT + voiceNo));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17 + offset, 101)), module, Sidofon::GATE_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(29 + offset, 101)), module, Sidofon::SYNC_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(41 + offset, 101)), module, Sidofon::RING_MOD_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(53 + offset, 101)), module, Sidofon::TEST_LIGHT + voiceNo));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(17 + offset, 117)), module, Sidofon::ATTACK_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(29 + offset, 117)), module, Sidofon::DECAY_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(41 + offset, 117)), module, Sidofon::SUSTAIN_LIGHT + voiceNo));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(53 + offset, 117)), module, Sidofon::RELEASE_LIGHT + voiceNo));

        // inputs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12 + offset, 64)), module, Sidofon::PITCH_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36 + offset, 64)), module, Sidofon::PULSE_WIDTH_INPUT + voiceNo));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12 + offset, 80)), module, Sidofon::WAVE_TRI_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24 + offset, 80)), module, Sidofon::WAVE_SAW_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36 + offset, 80)), module, Sidofon::WAVE_PULSE_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48 + offset, 80)), module, Sidofon::WAVE_NOISE_INPUT + voiceNo));
        
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12 + offset, 96)), module, Sidofon::GATE_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24 + offset, 96)), module, Sidofon::SYNC_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36 + offset, 96)), module, Sidofon::RING_MOD_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48 + offset, 96)), module, Sidofon::TEST_INPUT + voiceNo));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12 + offset, 112)), module, Sidofon::ATTACK_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24 + offset, 112)), module, Sidofon::DECAY_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36 + offset, 112)), module, Sidofon::SUSTAIN_INPUT + voiceNo));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(48 + offset, 112)), module, Sidofon::RELEASE_INPUT + voiceNo));
    }
};


Model* modelSidofon = createModel<Sidofon, SidofonWidget>("sidofon");