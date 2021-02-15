#pragma once
#include <rack.hpp>
#include "sid.h"

struct FilterRegs {
    enum Regs {
        CUTOFF_LO = 0,
        CUTOFF_HI,
        RES_FILT,
        MODE_VOL,
        NUM_REGS
    };

    uint8_t  regs[NUM_REGS];
    uint8_t  dirty;

    static constexpr uint16_t CUTOFF_MAX = 2047;
    static constexpr uint8_t  RESONANCE_MAX = 15;
    static constexpr uint8_t  VOLUME_MAX = 15;

    static constexpr uint8_t  FILT_EXT = 8;
    static constexpr uint8_t  MODE_LP = 0x10;
    static constexpr uint8_t  MODE_BP = 0x20;
    static constexpr uint8_t  MODE_HP = 0x40;
    static constexpr uint8_t  MODE_MASK = 0x70;
    static constexpr uint8_t  MODE_VOICE3OFF = 0x80;

    void realize(reSID::SID &sid);
    void reset();

    void setCutOff(uint16_t freq);
    void setResonance(uint8_t res);
    // voice_no=0..2
    void setFilterVoice(int voice_no, bool on);
    void setFilterExt(bool on);
    void setMode(uint8_t mode);
    void setVoice3Off(bool off);
    void setVolume(uint8_t volume);

    uint16_t getCutOff() { return regs[CUTOFF_HI] << 3 | regs[CUTOFF_LO]; }
    uint8_t  getResonance() { return regs[RES_FILT] >> 4; }
    bool     getFilterVoice(int voice_no) { 
        return (regs[RES_FILT] & (1 << voice_no)) == (1 << voice_no); }
    bool     getFilterExt() { return (regs[RES_FILT] & FILT_EXT) == FILT_EXT; }
    uint8_t  getMode() { return regs[MODE_VOL] & MODE_MASK; }
    bool     getVoice3Off() { return (regs[MODE_VOL] & MODE_VOICE3OFF) == MODE_VOICE3OFF; }
    uint8_t  getVolume() { return regs[MODE_VOL] & VOLUME_MAX; }
};
