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

    uint8_t  regs[NUM_REGS];
    uint8_t  dirty;

    static constexpr int      NUM_VOICES = 3;
    static constexpr uint16_t FREQ_MAX = 65535;
    static constexpr uint16_t PULSE_WIDTH_MAX = 4095;
    static constexpr uint8_t  ATTACK_MAX = 15;
    static constexpr uint8_t  DECAY_MAX = 15;
    static constexpr uint8_t  SUSTAIN_MAX = 15;
    static constexpr uint8_t  RELEASE_MAX = 15;

    static constexpr uint8_t  WAVE_TRIANGLE = 0x10;
    static constexpr uint8_t  WAVE_SAWTOOTH = 0x20;
    static constexpr uint8_t  WAVE_RECTANGLE = 0x40;
    static constexpr uint8_t  WAVE_NOISE = 0x80;
    static constexpr uint8_t  WAVE_MASK = 0xf0;

    static constexpr uint8_t  CTRL_GATE = 0x01;
    static constexpr uint8_t  CTRL_SYNC = 0x02;
    static constexpr uint8_t  CTRL_RING_MOD = 0x04;
    static constexpr uint8_t  CTRL_TEST = 0x08;    

    // voice_no=0..2
    void realize(reSID::SID &sid, int voice_no);
    void reset();

    void setFreq(uint16_t freq);
    void setPulseWidth(uint16_t pw);
    void setWaveform(uint8_t waveform);
    void setGate(bool on);
    void setSync(bool on);
    void setRingMod(bool on);
    void setTest(bool on);
    void setAttack(uint8_t attack);
    void setDecay(uint8_t decay);
    void setSustain(uint8_t sustain);
    void setRelease(uint8_t release);

    uint16_t getFreq() { return regs[FREQ_HI] << 8 | regs[FREQ_LO]; }
    uint16_t getPulseWidth() { return regs[PW_HI] << 8 | regs[PW_LO]; }
    uint8_t  getWaveform() { return regs[CONTROL] & WAVE_MASK; }
    bool     getGate() { return (regs[CONTROL] & CTRL_GATE) == CTRL_GATE; }
    bool     getSync() { return (regs[CONTROL] & CTRL_SYNC) == CTRL_SYNC; }
    bool     getRingMod() { return (regs[CONTROL] & CTRL_RING_MOD) == CTRL_RING_MOD; }
    bool     getTest() { return (regs[CONTROL] & CTRL_TEST) == CTRL_TEST; }
    uint8_t  getAttack() { return regs[ATTACK_DECAY] >> 4; }
    uint8_t  getDecay() { return regs[ATTACK_DECAY] & 0xf; }
    uint8_t  getSustain() { return regs[SUSTAIN_RELEASE] >> 4; }
    uint8_t  getRelease() { return regs[SUSTAIN_RELEASE] & 0xf; }
};
