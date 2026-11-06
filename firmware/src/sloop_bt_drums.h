/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP-BT drum lane mixer. Controls target sound lanes in the existing
 * fourth SLOOP track (NOT additional sequencer tracks).
 * Pure C99 functions to support host regression tests without hardware.
 */
#ifndef SLOOP_BT_DRUM_MIX_H
#define SLOOP_BT_DRUM_MIX_H
#include <stdint.h>
#define SLOOP_BT_LANES 16u
#define SLOOP_BT_STRIPS 4u
typedef struct {
    uint8_t gain[SLOOP_BT_LANES];  /* 0..127, default 127 */
    uint16_t mute, solo;           /* bit per actual drum sound lane */
    uint8_t assignment[SLOOP_BT_STRIPS]; /* physical strips 5..8 -> 0..15 */
} sloop_bt_drums_t;
static void sloop_bt_drums_reset(sloop_bt_drums_t *m)
{
    unsigned i;
    static const uint8_t initial[4] = {0, 2, 4, 3}; /* kick, snare, closed hat, clap */
    for(i=0;i<SLOOP_BT_LANES;i++) m->gain[i]=127u;
    for(i=0;i<SLOOP_BT_STRIPS;i++)m->assignment[i]=initial[i];
    m->mute=m->solo=0;
}
static unsigned sloop_bt_drum_strip_lane(const sloop_bt_drums_t *m,unsigned strip)
{
    return strip < 4u || strip >= 8u ? SLOOP_BT_LANES : m->assignment[strip-4u];
}
static void sloop_bt_drum_assign(sloop_bt_drums_t *m,unsigned strip,unsigned lane)
{
    if(strip>=4u && strip<8u && lane<SLOOP_BT_LANES) m->assignment[strip-4u]=(uint8_t)lane;
}
static void sloop_bt_drum_level(sloop_bt_drums_t *m,unsigned strip,unsigned level)
{
    unsigned lane=sloop_bt_drum_strip_lane(m,strip);
    if(lane>=SLOOP_BT_LANES)return;
    m->gain[lane]=(uint8_t)(level>127u?127u:level);
}
static void sloop_bt_drum_toggle_mute(sloop_bt_drums_t *m,unsigned strip)
{
    unsigned lane=sloop_bt_drum_strip_lane(m,strip);
    if(lane<SLOOP_BT_LANES)m->mute^=(uint16_t)(1u<<lane);
}
static void sloop_bt_drum_toggle_solo(sloop_bt_drums_t *m,unsigned strip)
{
    unsigned lane=sloop_bt_drum_strip_lane(m,strip);
    if(lane<SLOOP_BT_LANES)m->solo^=(uint16_t)(1u<<lane);
}
/* Default unity output is bit-exact vs stock SLOOP.
 * This gain stage runs only when explicitly altered by a mixer control.
 * Mixing code should smooth value changes before applying this function
 * in a real-time path to avoid discontinuities on sustained drum tails.
 */
static int32_t sloop_bt_drum_gain_sample(const sloop_bt_drums_t *m,unsigned lane,int32_t sample)
{
    unsigned gain;
    if(lane>=SLOOP_BT_LANES)return sample;
    if((m->mute>>lane)&1u)return 0;
    if(m->solo && !((m->solo>>lane)&1u))return 0;
    gain=m->gain[lane];
    if(gain==127u)return sample;
    if(gain==0u)return 0;
    /* DSP values are within signed 32-bit. Promote before scaling. */
    return (int32_t)((int64_t)sample*gain/127);
}
#endif
