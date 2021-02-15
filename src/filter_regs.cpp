#include "filter_regs.h"

#define DEBUG_REGS

void FilterRegs::realize(reSID::SID &sid)
{
    int offset = 21;
    int mask = 1;
    if(dirty == 0) {
        return;
    }
#ifdef DEBUG_SID
    printf("Update Mn:");
#endif
    for(int i=0;i<NUM_REGS;i++) {
        if((dirty & mask) == mask) {
            sid.write(offset, regs[i]);
#ifdef DEBUG_SID
            printf(" @%02x=%02x", offset, regs[i]);
#endif
        }
        offset++;
        mask<<=1;
    }
#ifdef DEBUG_SID
    printf("\n");
#endif
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
    regs[CUTOFF_LO] = (uint8_t)(freq & 7); // low bits: 0...2
    regs[CUTOFF_HI] = (uint8_t)((freq >> 3) & 0xff); // high bits 3...10
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
    uint8_t mask = FILT_EXT;
    if(on) {
        regs[RES_FILT] |= mask;
    } else {
        regs[RES_FILT] &= ~mask;
    }
    if(regs[RES_FILT] != old) {
        dirty |= 1<<RES_FILT;
    }
}

void FilterRegs::setMode(uint8_t mode)
{
    uint8_t old = regs[MODE_VOL];
    regs[MODE_VOL] = (mode & MODE_MASK) | (old & ~MODE_MASK);
    if(regs[MODE_VOL] != old) {
        dirty |= 1<<MODE_VOL;
    }
}

void FilterRegs::setVoice3Off(bool off)
{
    uint8_t old = regs[MODE_VOL];
    uint8_t mask = MODE_VOICE3OFF;
    if(off) {
        regs[MODE_VOL] |= mask;
    } else {
        regs[MODE_VOL] &= ~mask;
    }
    if(regs[MODE_VOL] != old) {
        dirty |= 1<<MODE_VOL;
    }
}

void FilterRegs::setVolume(uint8_t volume)
{
    uint8_t old = regs[MODE_VOL];
    regs[MODE_VOL] = (volume & VOLUME_MAX) | (old & ~VOLUME_MAX);
    if(regs[MODE_VOL] != old) {
        dirty |= 1<<MODE_VOL;
    }
}
