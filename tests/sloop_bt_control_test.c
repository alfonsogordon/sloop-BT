/* SPDX-License-Identifier: GPL-3.0-only
 * Host compile and functional test for actual sloop_bt_control.c adapter.
 * Stub only the SLOOP types/functions it touches. Unlike the pure
 * dispatcher tests, this includes the same adapter C file as the FM1.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/sloop_bt_drums.h"
#define NTRK 4u
#define TRK_DRUM 3u
#define P_LEVEL 0u
#define P_MUTE 1u
#define G_DRLVL 0u
typedef struct {int16_t p[2];} mock_track_t;
typedef struct {int16_t g[1]; uint8_t rec,sel,solo,playing;} mock_song_t;
typedef struct {uint8_t force;} mock_ui_t;
static mock_track_t trk[NTRK];
static mock_song_t song;
static mock_ui_t ui;
static uint8_t transport_req;
static sloop_bt_drums_t sloop_bt_mix;
static int32_t clamp(int32_t v,int32_t lo,int32_t hi) {return v<lo?lo:v>hi?hi:v;}
static void track_select(unsigned i) {if(i<NTRK)song.sel=(uint8_t)i;}
#include "../firmware/src/sloop_bt_control.c"

static void note(unsigned status,unsigned d1,unsigned d2)
{ sloop_bt_receive_midi((uint8_t)status,(uint8_t)d1,(uint8_t)d2); }

int main(void)
{
    smcbt_led_status_t leds;
    smcbt_led_msg_t ledmsg;
    unsigned index;
    sloop_bt_drums_reset(&sloop_bt_mix);
    trk[0].p[P_LEVEL]=80;trk[1].p[P_LEVEL]=70;trk[2].p[P_LEVEL]=90;
    song.g[G_DRLVL]=100;
    sloop_bt_control_connect();
    assert(sloop_bt_connected);
    /* Pitch-bend chan1: first point not picked; crossing sets track 1. */
    note(0xe0u,0,10);
    assert(trk[0].p[P_LEVEL]==80);
    note(0xe0u,0,82);
    assert(trk[0].p[P_LEVEL]==82);
    /* Drum master (channel 4) uses G_DRLVL, never trk[3].p[P_LEVEL]. */
    note(0xe3u,0,2);note(0xe3u,0,101);
    assert(song.g[G_DRLVL]==101);
    /* Fader 5 controls kick gain, fader 6 snare gain. */
    note(0xe4u,0,3);note(0xe4u,0,126);
    assert(sloop_bt_mix.gain[0]==126);
    note(0xe5u,0,2);note(0xe5u,0,100);
    assert(sloop_bt_mix.gain[2]==100);
    /* Row 0x10 = mute: strip 6 => snare, not trk[5]. */
    note(0x90u,0x15u,127u);
    assert(sloop_bt_mix.mute==(1u<<2));
    note(0x90u,0x11u,127u);
    assert(trk[1].p[P_MUTE]==1);
    /* Solo row 0x08. */
    note(0x90u,0x0cu,127u);
    assert(sloop_bt_mix.solo==(1u<<0));
    note(0x90u,0x09u,127u);
    assert(song.solo==(1u<<1));
    /* Selecting strip 8 selects drum bus and focuses clap. */
    note(0x90u,0x1fu,127u);
    assert(song.sel==TRK_DRUM && sloop_bt_focus_lane==3u);
    assert(sloop_bt_focus_strip==7u);
    /* REC strip 8 arms the one existing drum track. */
    note(0x90u,7u,127u);
    assert(song.rec==(1u<<TRK_DRUM));
    note(0x90u,0x5eu,127u);
    assert(transport_req==1u);
    note(0x90u,0x5du,127u);
    assert(transport_req==2u);
    sloop_bt_control_led_status(&leds);
    assert((leds.mute&(1u<<5)) && (leds.solo&(1u<<4)));
    assert((leds.rec&(1u<<7)) && leds.selected==7u);
    assert(smcbt_led_next(&sloop_bt_leds,&leds,&ledmsg,&index));
    smcbt_led_sent(&sloop_bt_leds,index,&ledmsg);
    /* An FM1 knob/edit changes gain: physical fader must re-pick. */
    trk[0].p[P_LEVEL]=25;
    sloop_bt_control_sync_levels();
    note(0xe0u,0,75);
    assert(trk[0].p[P_LEVEL]==25);
    note(0xe0u,0,24);
    assert(trk[0].p[P_LEVEL]==24);
    sloop_bt_control_disconnect();
    assert(sloop_bt_mix.solo==0 && song.solo==0);
    note(0xe0u,0,100);
    assert(trk[0].p[P_LEVEL]==24);
    puts("SLOOP-BT firmware adapter tests passed");
    return 0;
}
