#pragma once
#include <rack.hpp>
#include "sid.h"

struct VoiceRegs {
    enum Regs {
        FREQ_LO = 0,
        FREQ_HI,
        PW_LO,
        PW_HI,
        CONTROL,
        ATTACK_DECAY,
        SUSTAIN_RELEASE,
        NUM_REGS
    };

    enum Waveform {
        TRIANGLE = 0x10,
        SAWTOOTH = 0x20,
        RECTANGLE = 0x40,
        NOISE = 0x80,
        WAVEFORM_MASK = 0xf0
    };

    uint8_t  regs[NUM_REGS];
    uint8_t  dirty;

    enum class ControlReg : uint8_t {
        GATE = 1,
        SYNC = 2,
        RING_MOD = 4
    };

    static constexpr int      NUM_VOICES = 3;
    static constexpr uint16_t FREQ_MAX = 65535;
    static constexpr uint16_t PULSE_WIDTH_MAX = 4095;
    static constexpr uint8_t  ATTACK_MAX = 15;
    static constexpr uint8_t  DECAY_MAX = 15;
    static constexpr uint8_t  SUSTAIN_MAX = 15;
    static constexpr uint8_t  RELEASE_MAX = 15;

    // voice_no=0..2
    void realize(reSID::SID &sid, int voice_no);
    void reset();

    void setFreq(uint16_t freq);
    void setPulseWidth(uint16_t pw);
    void setWaveform(const Waveform &waveform);
    void setGate(bool on);
    void setSync(bool on);
    void setRingMod(bool on);
    void setAttack(uint8_t attack);
    void setDecay(uint8_t decay);
    void setSustain(uint8_t sustain);
    void setRelease(uint8_t release);
};
