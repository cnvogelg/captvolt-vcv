#include "filter_regs.h"

void FilterRegs::realize(reSID::SID &sid)
{
    int offset = 21;
    int mask = 1;
    for(int i=0;i<NUM_REGS;i++) {
        if((dirty & mask) == mask) {
            sid.write(offset, regs[i]);
            DEBUG("flt: @%02x=%02x", i, regs[i]);
        }
        offset++;
        mask<<=1;
    }
    dirty = 0;
}

void FilterRegs::reset()
{
    for(int i=0;i<NUM_REGS;i++) {
        regs[i] = 0;
    }
    // force update
    dirty = 0xff;
}

void FilterRegs::setCutOff(uint16_t freq)
{
    freq &= CUTOFF_MAX;
    uint8_t old_lo = regs[CUTOFF_LO];
    uint8_t old_hi = regs[CUTOFF_HI];
    regs[CUTOFF_LO] = (uint8_t)(freq & 0xff);
    regs[CUTOFF_HI] = (uint8_t)(freq >> 8);
    if(regs[CUTOFF_LO] != old_lo) {
        dirty |= (1<<CUTOFF_LO);
    }
    if(regs[CUTOFF_HI] != old_hi) {
        dirty |= (1<<CUTOFF_HI);
    }
}

void FilterRegs::setResonance(uint8_t res)
{
    res &= RESONANCE_MAX;
    uint8_t old = regs[RES_FILT];
    regs[RES_FILT] = (res << 4) | (old & 0xf);
    if(regs[RES_FILT] != old) {
        dirty |= (1<<RES_FILT);
    }
}

void FilterRegs::setFilterVoice(int voice_no, bool on)
{
    uint8_t old = regs[RES_FILT];
    uint8_t mask = 1 << (voice_no & 3);
    if(on) {
        regs[RES_FILT] |= mask;
    } else {
        regs[RES_FILT] &= ~mask;
    }
    if(regs[RES_FILT] != old) {
        dirty |= 1<<RES_FILT;
    }
}

void FilterRegs::setFilterExt(bool on)
{
    uint8_t old = regs[RES_FILT];
    uint8_t mask = 0x08;
    if(on) {
        regs[RES_FILT] |= mask;
    } else {
        regs[RES_FILT] &= ~mask;
    }
    if(regs[RES_FILT] != old) {
        dirty |= 1<<RES_FILT;
    }
}

void FilterRegs::setMode(const FilterMode &mode)
{
    uint8_t mode_mask = ((uint8_t)mode) << 4;
    uint8_t all_mask = 0x70;
    uint8_t old = regs[CONTROL];
    regs[CONTROL] = mode_mask | (old & ~all_mask);
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}

void FilterRegs::setVoice3Off(bool off)
{
    uint8_t old = regs[CONTROL];
    uint8_t mask = 0x80;
    if(off) {
        regs[CONTROL] |= mask;
    } else {
        regs[CONTROL] &= ~mask;
    }
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}

void FilterRegs::setVolume(uint8_t volume)
{
    volume &= VOLUME_MAX;
    uint8_t all_mask = 0xf;
    uint8_t old = regs[CONTROL];
    regs[CONTROL] = volume | (old & ~all_mask);
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}
