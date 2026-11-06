/* SPDX-License-Identifier: GPL-3.0-only
 * Single-producer (BLE RX IRQ), single-consumer (FM1 main-loop) MIDI FIFO.
 * No memory allocation or audio calls. Power of two, bounded.
 */
#ifndef SLOOP_BT_MIDI_RING_H
#define SLOOP_BT_MIDI_RING_H
#include <stdint.h>
#define SLOOP_BT_MIDI_Q_SIZE 64u
typedef struct {
    volatile uint32_t head, tail, dropped;
    uint32_t packets[SLOOP_BT_MIDI_Q_SIZE];
} sloop_bt_midi_ring_t;
static inline void sloop_bt_midi_ring_reset(sloop_bt_midi_ring_t *q)
{
    q->head=q->tail=q->dropped=0u;
}
/* One producer on FM1 BLE IRQ; returns 0 and increments drops if full.
 * Order the packet store before publishing the head index.
 */
static inline int sloop_bt_midi_ring_push(sloop_bt_midi_ring_t *q,uint32_t pkt)
{
    uint32_t h=q->head;
    if(h-q->tail>=SLOOP_BT_MIDI_Q_SIZE){
        q->dropped++;
        return 0;
    }
    q->packets[h & (SLOOP_BT_MIDI_Q_SIZE-1u)]=pkt;
    __asm__ __volatile__("" ::: "memory");
    q->head=h+1u;
    return 1;
}
/* One consumer in SLOOP's main loop, outside the radio IRQ. */
static inline int sloop_bt_midi_ring_pop(sloop_bt_midi_ring_t *q,uint32_t *pkt)
{
    uint32_t t=q->tail;
    if(t==q->head)return 0;
    __asm__ __volatile__("" ::: "memory");
    *pkt=q->packets[t & (SLOOP_BT_MIDI_Q_SIZE-1u)];
    q->tail=t+1u;
    return 1;
}
#endif
