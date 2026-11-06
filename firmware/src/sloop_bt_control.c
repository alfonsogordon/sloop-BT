/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP-BT firmware adapter (included from felucca.c AFTER ui_input.c).
 * This is an actual adapter to SLOOP's live state, NOT a radio driver.
 * It is built only with SLOOP_BT_CONTROL=1 and is never active in
 * the stock build. All entry points are main-loop only.
 */
#include "sloop_bt_dispatch.h"
#include "smcbt_leds.h"
/* The Optimist radio stack supplies its own BLE-MIDI wire codec.
 * Avoid linking a duplicate Hortator parser into the measured port.
 * Legacy SLOOP-BT host tests keep their original BLE codec.
 */
#if defined(SLOOP_BT_OPEN_MEASURE) && SLOOP_BT_OPEN_MEASURE
#define SLOOP_BT_USE_OPTIMIST 1
#else
#define SLOOP_BT_USE_OPTIMIST 0
#endif
#if !SLOOP_BT_USE_OPTIMIST
#include "sloop_bt_ble/ble_midi.c"
static ble_midi_t sloop_bt_ble_in;
#endif

static smcbt_state_t sloop_bt_controller;
static uint8_t sloop_bt_connected;
static uint8_t sloop_bt_focus_lane;
static uint8_t sloop_bt_focus_strip;
static uint8_t sloop_bt_solo_initial, sloop_bt_solo_touched;
static smcbt_led_state_t sloop_bt_leds;

static uint8_t sloop_bt_track_level(unsigned i)
{
    if(i >= NTRK) return 127u;
    return (uint8_t)clamp(i == TRK_DRUM ? song.g[G_DRLVL] : trk[i].p[P_LEVEL],0,127);
}
static void sloop_bt_control_connect(void)
{
    uint8_t level[SMCBT_TRACKS];
    unsigned i;
    for(i=0;i<4u;i++)level[i]=sloop_bt_track_level(i);
    for(i=4u;i<8u;i++) {
        unsigned lane=sloop_bt_drum_strip_lane(&sloop_bt_mix,i);
        level[i]=lane<16u?sloop_bt_mix.gain[lane]:127u;
    }
    smcbt_reset(&sloop_bt_controller,level);
    sloop_bt_solo_initial=song.solo;
    sloop_bt_solo_touched=0;
#if !SLOOP_BT_USE_OPTIMIST
    ble_midi_reset(&sloop_bt_ble_in);
#endif
    smcbt_led_reset(&sloop_bt_leds);
    sloop_bt_connected=1u;
}
static void sloop_bt_control_disconnect(void)
{
    sloop_bt_connected=0u;
    /* A remote solo may otherwise leave the project unexpectedly muted. */
    sloop_bt_mix.solo=0;
    song.solo=(uint8_t)((song.solo & (uint8_t)~sloop_bt_solo_touched) |
                        (sloop_bt_solo_initial & sloop_bt_solo_touched));
    sloop_bt_solo_touched=0;
}
static void sloop_bt_level(void *unused,unsigned i,unsigned level)
{
    (void)unused;
    if(i>=NTRK)return;
    if(i==TRK_DRUM)song.g[G_DRLVL]=(int16_t)level;
    else trk[i].p[P_LEVEL]=(int16_t)level;
    ui.force=1;
}
static void sloop_bt_mute(void *unused,unsigned i)
{
    (void)unused;
    if(i<NTRK) {
        trk[i].p[P_MUTE]=(int16_t)!trk[i].p[P_MUTE];
        ui.force=1;
    }
}
static void sloop_bt_solo(void *unused,unsigned i)
{
    (void)unused;
    if(i<NTRK) {
        song.solo^=(uint8_t)(1u<<i);
        sloop_bt_solo_touched|=(uint8_t)(1u<<i);
        ui.force=1;
    }
}
static void sloop_bt_select(void *unused,unsigned i)
{
    (void)unused;
    if(i<NTRK)track_select(i);
}
static void sloop_bt_arm(void *unused,unsigned i)
{
    (void)unused;
    /* SLOOP's recording is single-track; do not invent extra tracks. */
    if(i<NTRK) {
        track_select(i);
        song.rec=(uint8_t)(song.rec==(1u<<i)?0u:(1u<<i));
        ui.force=1;
    }
}
static void sloop_bt_transport(void *unused,unsigned op)
{
    (void)unused;
    if(op==1u) {
        if(!song.playing)transport_req=1u;
    } else if(op==2u) {
        transport_req=2u;
    } else if(op==3u) {
        /* A single REC press arms/disarms the selected track. */
        sloop_bt_arm(0,song.sel);
    }
}
static void sloop_bt_knob(void *unused,unsigned n,int delta)
{
    (void)unused;(void)n;(void)delta;
    /* Not connected until the SLOOP parameter menu mapping is verified. */
}
static void sloop_bt_focus(void *unused,unsigned lane)
{
    (void)unused;
    if(lane<16u) {
        sloop_bt_focus_lane=(uint8_t)lane;
        ui.force=1;
    }
}
static const sloop_bt_ops_t sloop_bt_ops={
    sloop_bt_level,sloop_bt_mute,sloop_bt_solo,sloop_bt_select,
    sloop_bt_arm,sloop_bt_transport,sloop_bt_knob,sloop_bt_focus
};

/* Called by the eventual BLE-MIDI transport's main-loop notification
 * handler. Never from TIMER5 or the audio interrupt. */
static void sloop_bt_receive_midi(uint8_t status,uint8_t d1,uint8_t d2)
{
    smcbt_event_t e;
    if(!sloop_bt_connected)return;
    if(!smcbt_midi(&sloop_bt_controller,status,d1,d2,&e))return;
    if(e.kind==SMCBT_SELECT||e.kind==SMCBT_REC_TRACK)
        sloop_bt_focus_strip=e.track;
    sloop_bt_dispatch(&e,&sloop_bt_mix,&sloop_bt_ops,0);
    if(e.kind==SMCBT_DRUM_LEVEL||e.kind==SMCBT_MUTE||e.kind==SMCBT_SOLO)
        ui.force=1;
}
/* Native Optimist BLE stack uses packed USB-MIDI event words:
 * CIN | status<<8 | data1<<16 | data2<<24. The radio's ISR must
 * enqueue these to a bounded ring, and the main loop drains the ring
 * through this function. Never run SLOOP UI work in the radio IRQ.
 */
static void sloop_bt_midi_usb_packet(uint32_t p)
{
    unsigned cin=p&15u;
    if(cin<8u || cin>14u)return; /* SMC DAW: channel MIDI only */
    sloop_bt_receive_midi((uint8_t)(p>>8),(uint8_t)(p>>16),(uint8_t)(p>>24));
}
/* BLE central adapter: only call from BLE service on SLOOP's main
 * loop after subscription. Decode a standard BLE-MIDI notification
 * and hand messages to the controller dispatcher. Not a radio driver.
 */
#if !SLOOP_BT_USE_OPTIMIST
static void sloop_bt_ble_packet(void *ctx,uint32_t packet)
{
    (void)ctx;
    if(((packet>>8)&0xf0u)==0xf0u)return; /* SMC DAW ignores system bytes */
    sloop_bt_receive_midi((uint8_t)(packet>>8),(uint8_t)(packet>>16),(uint8_t)(packet>>24));
}
static void sloop_bt_ble_notification(const uint8_t *payload,uint32_t length)
{
    if(!sloop_bt_connected)return;
    ble_midi_parse(&sloop_bt_ble_in,payload,length,sloop_bt_ble_packet,0);
}
#endif
/* The four synth/drum bus level faders and four drum-lane faders must
 * re-pick when their values change at the device (knobs, load, editor).
 * Called once per main-loop frame, not during the audio render.
 */
static void sloop_bt_control_sync_levels(void)
{
    unsigned i;
    for(i=0;i<4u;i++)
        smcbt_level_changed(&sloop_bt_controller,i,sloop_bt_track_level(i));
    for(i=4u;i<8u;i++) {
        unsigned lane=sloop_bt_drum_strip_lane(&sloop_bt_mix,i);
        if(lane<16u)
            smcbt_level_changed(&sloop_bt_controller,i,sloop_bt_mix.gain[lane]);
    }
}
/* The pending LEDs are encoded by smcbt_led_next; the transport must
 * call smcbt_led_sent only if its send queue accepted the message.
 * Per-drum REC reflects the existing single drum recording track.
 */
static void sloop_bt_control_led_status(smcbt_led_status_t *out)
{
    unsigned i;
    out->mute=out->solo=out->rec=0;
    for(i=0;i<4u;i++) {
        out->mute|=(uint8_t)((trk[i].p[P_MUTE]!=0u)<<i);
        out->solo|=(uint8_t)(((song.solo>>i)&1u)<<i);
        out->rec|=(uint8_t)(((song.rec>>i)&1u)<<i);
    }
    for(i=4u;i<8u;i++) {
        unsigned lane=sloop_bt_drum_strip_lane(&sloop_bt_mix,i);
        if(lane>=16u)continue;
        out->mute|=(uint8_t)(((sloop_bt_mix.mute>>lane)&1u)<<i);
        out->solo|=(uint8_t)(((sloop_bt_mix.solo>>lane)&1u)<<i);
        out->rec|=(uint8_t)(((song.rec>>TRK_DRUM)&1u)<<i);
    }
    out->selected=(uint8_t)(song.sel==TRK_DRUM && sloop_bt_focus_strip>=4u
                                 ? sloop_bt_focus_strip : song.sel);
    out->playing=song.playing!=0;
    out->recording=song.rec!=0;
}

/* Encode ONE pending LED update as a standards-compliant BLE-MIDI
 * notification payload. A radio sender may queue it when connected;
 * crucially, do NOT mark it delivered unless the queue accepted it.
 * This is main-loop code only, never called by the audio interrupt.
 */
#if !SLOOP_BT_USE_OPTIMIST
static uint32_t sloop_bt_led_packet(uint32_t timestamp_ms,uint8_t out[8],unsigned *index)
{
    smcbt_led_status_t status;
    smcbt_led_msg_t msg;
    uint8_t midi[1][3], lengths[1]={3};
    uint32_t taken;
    if(!sloop_bt_connected)return 0;
    sloop_bt_control_led_status(&status);
    if(!smcbt_led_next(&sloop_bt_leds,&status,&msg,index))return 0;
    midi[0][0]=msg.status;
    midi[0][1]=msg.note;
    midi[0][2]=msg.velocity;
    return ble_midi_pack(midi,lengths,1,timestamp_ms,out,8u,&taken);
}
#endif
static void sloop_bt_led_queued(unsigned index,uint8_t on)
{
    smcbt_led_msg_t msg;
    msg.status=0x90u;
    msg.note=smcbt_led_note(index);
    msg.velocity=on?127u:0u;
    smcbt_led_sent(&sloop_bt_leds,index,&msg);
}
