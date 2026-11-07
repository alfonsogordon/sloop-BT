/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP-BT SMC logical dispatch: converts eight physical mixer strips
 * into 3 synth tracks, drum bus (track 4), and four drum sound lanes.
 * No Bluetooth dependency or audio-IRQ calls. Main loop only.
 */
#ifndef SLOOP_BT_DISPATCH_H
#define SLOOP_BT_DISPATCH_H
#include <stdint.h>
#include "smcbt.h"
#include "sloop_bt_drums.h"
typedef struct {
    void (*track_level)(void *,unsigned,unsigned);
    void (*track_mute)(void *,unsigned);
    void (*track_solo)(void *,unsigned);
    void (*track_select)(void *,unsigned);
    void (*track_record)(void *,unsigned);
    void (*transport)(void *,unsigned); /* 1 play 2 stop 3 REC */
    void (*knob)(void *,unsigned,int); /* relative step, index 0..7 */
    void (*drum_focus)(void *,unsigned); /* sound lane 0..15 */
} sloop_bt_ops_t;
/* All callbacks optional so host tests can record just the actions
 * they care about; the firmware adapter must provide complete ones.
 */
static void sloop_bt_dispatch(const smcbt_event_t *e,sloop_bt_drums_t *drums,
                             const sloop_bt_ops_t *ops,void *ctx)
{
    unsigned strip=e->track;
    unsigned lane=sloop_bt_drum_strip_lane(drums,strip);
    int on_drum_lane=strip>=4u && strip<8u;
    switch(e->kind) {
    case SMCBT_LEVEL:
        if(strip<4u && ops->track_level)ops->track_level(ctx,strip,e->value);
        break;
    case SMCBT_DRUM_LEVEL:
        sloop_bt_drum_level(drums,strip,e->value);
        break;
    case SMCBT_MUTE:
        if(on_drum_lane)sloop_bt_drum_toggle_mute(drums,strip);
        else if(strip<4u && ops->track_mute)ops->track_mute(ctx,strip);
        break;
    case SMCBT_SOLO:
        if(on_drum_lane)sloop_bt_drum_toggle_solo(drums,strip);
        else if(strip<4u && ops->track_solo)ops->track_solo(ctx,strip);
        break;
    case SMCBT_SELECT:
        if(on_drum_lane) {
            if(ops->track_select)ops->track_select(ctx,3u);
            if(ops->drum_focus)ops->drum_focus(ctx,lane);
        } else if(strip<4u && ops->track_select)ops->track_select(ctx,strip);
        break;
    case SMCBT_REC_TRACK:
        if(on_drum_lane){
            if(ops->track_select)ops->track_select(ctx,3u);
            if(ops->drum_focus)ops->drum_focus(ctx,lane);
            if(ops->track_record)ops->track_record(ctx,3u);
        } else if(strip<4u && ops->track_record)ops->track_record(ctx,strip);
        break;
    case SMCBT_PLAY:
        if(ops->transport)ops->transport(ctx,1u);
        break;
    case SMCBT_STOP:
        if(ops->transport)ops->transport(ctx,2u);
        break;
    case SMCBT_REC:
        if(ops->transport)ops->transport(ctx,3u);
        break;
    case SMCBT_KNOB:
        if(ops->knob)ops->knob(ctx,e->index,e->delta);
        break;
    default: break; /* bank buttons gain actions with focused-strip state */
    }
}
#endif
