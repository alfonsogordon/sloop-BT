/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/sloop_bt_midi_ring.h"
int main(void)
{
    sloop_bt_midi_ring_t q={0};
    uint32_t value;
    unsigned i;
    sloop_bt_midi_ring_reset(&q);
    assert(!sloop_bt_midi_ring_pop(&q,&value));
    for(i=0;i<SLOOP_BT_MIDI_Q_SIZE;i++)
        assert(sloop_bt_midi_ring_push(&q,0x90000009u+i));
    assert(!sloop_bt_midi_ring_push(&q,1u));
    assert(q.dropped==1u);
    for(i=0;i<SLOOP_BT_MIDI_Q_SIZE;i++){
        assert(sloop_bt_midi_ring_pop(&q,&value));
        assert(value==0x90000009u+i);
    }
    assert(!sloop_bt_midi_ring_pop(&q,&value));
    for(i=0;i<SLOOP_BT_MIDI_Q_SIZE*3u;i++){
        assert(sloop_bt_midi_ring_push(&q,0x0e000000u+i));
        assert(sloop_bt_midi_ring_pop(&q,&value));
        assert(value==0x0e000000u+i);
    }
    assert(q.dropped==1u);
    puts("BLE IRQ/main-loop ring tests passed");
    return 0;
}
