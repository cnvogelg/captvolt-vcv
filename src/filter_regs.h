#pragma once
#include <rack.hpp>
#include "sid.h"

enum class FilterMode : uint8_t {
    Off = 0,
    HighPass = 0x40,
    BandPass = 0x20,
    LowPass = 0x10,
    ALL = 0x70
};

struct FilterRegs {
    enum Regs {
        CUTOFF_LO = 0,
        CUTOFF_HI,
        RES_FILT,
        CONTROL,
        NUM_REGS
    };

    uint8_t  regs[NUM_REGS];
    uint8_t  dirty;

    static constexpr uint16_t CUTOFF_MAX = 2047;
    static constexpr uint8_t  RESONANCE_MAX = 15;
    static constexpr uint8_t  VOLUME_MAX = 15;

    void realize(reSID::SID &sid);
    void reset();

    void setCutOff(uint16_t freq);
    void setResonance(uint8_t res);
    // voice_no=0..2
    void setFilterVoice(int voice_no, bool on);
    void setFilterExt(bool on);
    void setMode(const FilterMode &mode);
    void setVoice3Off(bool off);
    void setVolume(uint8_t volume);
};
