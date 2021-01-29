#include "voice_regs.h"

void VoiceRegs::realize(reSID::SID &sid, int voice_no)
{
    int offset = voice_no * NUM_REGS;
    int mask = 1;
    if(dirty == 0) {
        return;
    }
#ifdef DEBUG_SID
    printf("Update #%d:", voice_no);
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

void VoiceRegs::reset()
{
    for(int i=0;i<NUM_REGS;i++) {
        regs[i] = 0;
    }
    // force update
    dirty = 0xff;
}

// 0..65535
bool VoiceRegs::setFreq(uint16_t freq)
{
    uint8_t old_lo = regs[FREQ_LO];
    uint8_t old_hi = regs[FREQ_HI];
    regs[FREQ_LO] = (uint8_t)(freq & 0xff);
    regs[FREQ_HI] = (uint8_t)(freq >> 8);
    bool d = false;
    if(regs[FREQ_LO] != old_lo) {
        dirty |= (1<<FREQ_LO);
        d = true;
    }
    if(regs[FREQ_HI] != old_hi) {
        dirty |= (1<<FREQ_HI);
        d = true;
    }
    return d;
}

// 0..4095
void VoiceRegs::setPulseWidth(uint16_t pw)
{
    pw &= PULSE_WIDTH_MAX;
    uint8_t old_lo = regs[PW_LO];
    uint8_t old_hi = regs[PW_HI];
    regs[PW_LO] = (uint8_t)(pw & 0xff);
    regs[PW_HI] = (uint8_t)((pw >> 8) & 0xf);
    if(regs[PW_LO] != old_lo) {
        dirty |= (1<<PW_LO);
    }
    if(regs[PW_HI] != old_hi) {
        dirty |= (1<<PW_HI);
    }
}

void VoiceRegs::setWaveform(uint8_t waveform)
{
    uint8_t old = regs[CONTROL];
    regs[CONTROL] &= ~WAVE_MASK;
    regs[CONTROL] |= (waveform & WAVE_MASK);
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}

void VoiceRegs::setGate(bool on)
{
    uint8_t old = regs[CONTROL];
    if(on) {
        regs[CONTROL] |= CTRL_GATE;
    } else {
        regs[CONTROL] &= ~CTRL_GATE;
    }
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}

void VoiceRegs::setSync(bool on)
{
    uint8_t old = regs[CONTROL];
    if(on) {
        regs[CONTROL] |= CTRL_SYNC;
    } else {
        regs[CONTROL] &= ~CTRL_SYNC;
    }
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}

void VoiceRegs::setRingMod(bool on)
{
    uint8_t old = regs[CONTROL];
    if(on) {
        regs[CONTROL] |= CTRL_RING_MOD;
    } else {
        regs[CONTROL] &= ~CTRL_RING_MOD;
    }
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}

void VoiceRegs::setTest(bool on)
{
    uint8_t old = regs[CONTROL];
    if(on) {
        regs[CONTROL] |= CTRL_TEST;
    } else {
        regs[CONTROL] &= ~CTRL_TEST;
    }
    if(regs[CONTROL] != old) {
        dirty |= 1<<CONTROL;
    }
}

void VoiceRegs::setAttack(uint8_t attack)
{
    uint8_t old = regs[ATTACK_DECAY];
    attack &= ATTACK_MAX;
    regs[ATTACK_DECAY] = (attack << 4) | (regs[ATTACK_DECAY] & 0xf);
    if(regs[ATTACK_DECAY] != old) {
        dirty |= 1<<ATTACK_DECAY;
    }
}

void VoiceRegs::setDecay(uint8_t decay)
{
    uint8_t old = regs[ATTACK_DECAY];
    decay &= DECAY_MAX;
    regs[ATTACK_DECAY] = decay | (regs[ATTACK_DECAY] & 0xf0);
    if(regs[ATTACK_DECAY] != old) {
        dirty |= 1<<ATTACK_DECAY;
    }
}

void VoiceRegs::setSustain(uint8_t sustain)
{
    uint8_t old = regs[SUSTAIN_RELEASE];
    sustain &= SUSTAIN_MAX;
    regs[SUSTAIN_RELEASE] = (sustain << 4) | (regs[SUSTAIN_RELEASE] & 0xf);
    if(regs[SUSTAIN_RELEASE] != old) {
        dirty |= 1<<SUSTAIN_RELEASE;
    }
}

void VoiceRegs::setRelease(uint8_t release)
{
    uint8_t old = regs[SUSTAIN_RELEASE];
    release &= RELEASE_MAX;
    regs[SUSTAIN_RELEASE] = release | (regs[SUSTAIN_RELEASE] & 0xf0);
    if(regs[SUSTAIN_RELEASE] != old) {
        dirty |= 1<<SUSTAIN_RELEASE;
    }
}
