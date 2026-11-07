/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/sloop_bt_drums.h"
int main(void)
{
    sloop_bt_drums_t m;
    int i;
    sloop_bt_drums_reset(&m);
    for(i=0;i<16;i++) assert(m.gain[i]==127);
    assert(m.assignment[0]==0 && m.assignment[1]==2 &&
           m.assignment[2]==4 && m.assignment[3]==3);
    for(i=0;i<16;i++)
        assert(sloop_bt_drum_gain_sample(&m,(unsigned)i,1234567)==1234567);
    sloop_bt_drum_level(&m,4,64);
    assert(m.gain[0]==64);
    assert(sloop_bt_drum_gain_sample(&m,0,12700)==6400);
    assert(sloop_bt_drum_gain_sample(&m,2,12700)==12700);
    sloop_bt_drum_toggle_mute(&m,4);
    assert(sloop_bt_drum_gain_sample(&m,0,12700)==0);
    sloop_bt_drum_toggle_mute(&m,4);
    assert(sloop_bt_drum_gain_sample(&m,0,12700)==6400);
    sloop_bt_drum_toggle_solo(&m,5); /* snare only */
    assert(sloop_bt_drum_gain_sample(&m,0,12700)==0);
    assert(sloop_bt_drum_gain_sample(&m,2,12700)==12700);
    sloop_bt_drum_toggle_solo(&m,5);
    assert(sloop_bt_drum_gain_sample(&m,0,12700)==6400);
    sloop_bt_drum_assign(&m,7,15);
    sloop_bt_drum_level(&m,7,0);
    assert(m.gain[15]==0);
    assert(sloop_bt_drum_gain_sample(&m,15,12700)==0);
    assert(sloop_bt_drum_strip_lane(&m,8)==16);
    sloop_bt_drum_level(&m,8,50);
    assert(m.gain[15]==0);
    puts("SLOOP-BT drum-lane mixer tests passed");
    return 0;
}
