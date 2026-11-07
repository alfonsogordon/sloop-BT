/* SPDX-License-Identifier: GPL-3.0-only
 * Host tests for Hortator BLE-MIDI codec, imported for SLOOP-BT.
 * cc -std=c99 -Wall -Wextra -Werror tests/sloop_bt_ble_midi_test.c \
 *    firmware/src/sloop_bt_ble/ble_midi.c -o /tmp/sloop_bt_ble_midi_test
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/sloop_bt_ble/ble_midi.h"
#include "../firmware/src/smcbt.h"
typedef struct {unsigned count;uint32_t pkt[12];} events_t;
static void take(void *arg,uint32_t pkt)
{
    events_t *v=arg;
    assert(v->count<12u);
    v->pkt[v->count++]=pkt;
}
static int event_to_smc(smcbt_state_t *s,uint32_t pkt,smcbt_event_t *e)
{
    return smcbt_midi(s,(uint8_t)(pkt>>8),(uint8_t)(pkt>>16),(uint8_t)(pkt>>24),e);
}
int main(void)
{
    ble_midi_t midi;
    smcbt_state_t smc;
    smcbt_event_t e;
    events_t events={0};
    uint8_t start[8]={80,70,90,100,127,127,127,127};
    const uint8_t fader[] = {0x80,0x80,0xe0,0,82};
    const uint8_t button[] = {0x80,0x81,0x90,0x15,127};
    const uint8_t running[] = {0x80,0x82,0x90,0x10,127,0x83,0x11,127};
    uint8_t packed[32];
    const uint8_t output[2][3]={{0x90,0x15,127},{0x90,0x15,0}};
    const uint8_t outlen[2]={3,3};
    uint32_t taken,used;
    ble_midi_reset(&midi);
    smcbt_reset(&smc,start);
    ble_midi_parse(&midi,fader,sizeof fader,take,&events);
    assert(events.count==1u);
    assert(event_to_smc(&smc,events.pkt[0],&e));
    assert(e.kind==SMCBT_LEVEL && e.track==0u && e.value==82u);
    ble_midi_parse(&midi,button,sizeof button,take,&events);
    assert(events.count==2u);
    assert(event_to_smc(&smc,events.pkt[1],&e));
    assert(e.kind==SMCBT_MUTE && e.track==5u);
    ble_midi_parse(&midi,running,sizeof running,take,&events);
    assert(events.count==4u);
    assert(event_to_smc(&smc,events.pkt[2],&e) && e.track==0u);
    assert(event_to_smc(&smc,events.pkt[3],&e) && e.track==1u);
    used=ble_midi_pack(output,outlen,2,1000,packed,sizeof packed,&taken);
    assert(taken==2u && used==9u && packed[0]==(0x80u|(1000u>>7)));
    assert(packed[1]==(0x80u|(1000u&127u)) && packed[2]==0x90u);
    ble_midi_reset(&midi);
    events.count=0;
    ble_midi_parse(&midi,packed,used,take,&events);
    assert(events.count==2u);
    assert((events.pkt[0]>>8&255u)==0x90u && (events.pkt[1]>>24&255u)==0u);
    puts("SLOOP-BT BLE MIDI wire codec tests passed");
    return 0;
}
