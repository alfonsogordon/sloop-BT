/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP-BT: SMC-Mixer MIDI event decoder and eight-strip fader pickup.
 *
 * The SMC-Mixer in DAW mode sends faders as pitch bend on channels 1..8,
 * knobs as CC 16..23 on channel 1, and Mackie buttons as note-ons.
 * This file deliberately has NO Bluetooth or SLOOP dependencies. A BLE
 * adapter will pass decoded MIDI messages to smcbt_midi(), and apply
 * the resulting actions on SLOOP's main loop. The original sound
 * engine, sequencer and MIDI Learn are left unchanged.
 *
 * Wire format and pickup are based on DeadActive/hortator's GPL-3.0
 * firmware/src/smc.c, adapted for SLOOP's three synth tracks, drum bus and four drum lanes.
 */
#ifndef SLOOP_SMC_BT_H
#define SLOOP_SMC_BT_H
#include <stdint.h>

#define SMCBT_TRACKS 8u
#define SMCBT_SYNTH_TRACKS 3u
/* Physical strips: 0-2 synth, 3 drum bus, 4-7 four drum lanes. */
static const uint8_t smcbt_default_drum_lanes[4] = {0u, 2u, 4u, 3u}; /* kick, snare, hat, clap */
enum smcbt_action {
    SMCBT_NONE = 0,
    SMCBT_LEVEL,       /* track 0..2: synth level, 3: G_DRLVL drum bus */
    SMCBT_DRUM_LEVEL,  /* track 4..7: velocity/gain for a chosen drum lane */
    SMCBT_MUTE,        /* toggle track's mute */
    SMCBT_SOLO,        /* toggle track's solo */
    SMCBT_SELECT,      /* select track */
    SMCBT_REC_TRACK,   /* arm/disarm track */
    SMCBT_PLAY, SMCBT_STOP, SMCBT_REC,
    SMCBT_KNOB,        /* index 0..7, signed delta */
    SMCBT_PREV, SMCBT_NEXT
};
typedef struct {
    uint8_t kind, track, value, index;
    int8_t delta;
} smcbt_event_t;
typedef struct {
    uint8_t level[SMCBT_TRACKS], last[SMCBT_TRACKS];
    uint8_t seen, picked;
} smcbt_state_t;

/* On connecting, every fader must cross or reach its current software level. */
static inline void smcbt_reset(smcbt_state_t *s, const uint8_t levels[SMCBT_TRACKS])
{
    unsigned i;
    s->seen = s->picked = 0;
    for (i = 0; i < SMCBT_TRACKS; i++) {
        s->level[i] = levels[i];
        s->last[i] = 0;
    }
}

/* Called if the firmware (rather than the controller) changed a level. */
static inline void smcbt_level_changed(smcbt_state_t *s, unsigned track, uint8_t value)
{
    if (track >= SMCBT_TRACKS) return;
    if (value > 127u) value = 127u;
    if (s->level[track] != value) {
        s->level[track] = value;
        s->picked &= (uint8_t)~(1u << track);
    }
}

/* 1 = usable action, 0 = ignored or waiting for pickup.
 * The MIDI adapter supplies status, data1 and data2, not BLE packets.
 */
static inline int smcbt_midi(smcbt_state_t *s, uint8_t status, uint8_t d1, uint8_t d2,
                      smcbt_event_t *ev)
{
    unsigned ch = status & 15u, type = status & 0xf0u;
    ev->kind = SMCBT_NONE;
    ev->track = ev->value = ev->index = 0;
    ev->delta = 0;
    if (type == 0xe0u && ch < SMCBT_TRACKS) {
        unsigned b = 1u << ch;
        unsigned value = (((unsigned)(d2 & 127u) << 7) | (d1 & 127u)) >> 7;
        unsigned current = s->level[ch];
        int crossed = (s->seen & b) && ((s->last[ch] < current) != (value < current));
        s->last[ch] = (uint8_t)value;
        s->seen |= (uint8_t)b;
        if (!(s->picked & b)) {
            if (!crossed && (value + 2u < current || value > current + 2u))
                return 0;
            s->picked |= (uint8_t)b;
        }
        if (value == current) return 0;
        s->level[ch] = (uint8_t)value;
        ev->kind = ch < 4u ? SMCBT_LEVEL : SMCBT_DRUM_LEVEL;
        ev->track = (uint8_t)ch;
        ev->value = (uint8_t)value;
        return 1;
    }
    if (type == 0xb0u && ch == 0 && d1 >= 16u && d1 <= 23u) {
        unsigned clicks = d2 & 63u;
        if (!clicks) return 0;
        ev->kind = SMCBT_KNOB;
        ev->index = (uint8_t)(d1 - 16u);
        ev->delta = (int8_t)((d2 & 64u) ? -(int)clicks : (int)clicks);
        return 1;
    }
    /* The SMC DAW layout sends button *presses* as velocity > 0. */
    if (type != 0x90u || ch || !d2) return 0;
    if (d1 < 32u) {
        ev->track = d1 & 7u;
        if (ev->track >= SMCBT_TRACKS) return 0;
        ev->kind = (uint8_t)(d1 < 8u ? SMCBT_REC_TRACK :
                            d1 < 16u ? SMCBT_SOLO :
                            d1 < 24u ? SMCBT_MUTE : SMCBT_SELECT);
        return 1;
    }
    switch (d1) {
    case 0x5e: ev->kind = SMCBT_PLAY; break;
    case 0x5d: ev->kind = SMCBT_STOP; break;
    case 0x5f: ev->kind = SMCBT_REC; break;
    case 0x2e: case 0x30: ev->kind = SMCBT_PREV; break;
    case 0x2f: case 0x31: ev->kind = SMCBT_NEXT; break;
    default: return 0;
    }
    return 1;
}
#endif
