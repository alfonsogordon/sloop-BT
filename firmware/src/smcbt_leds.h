/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP-BT: Mackie/SMC LED feedback encoding, derived from the
 * message layout documented in DeadActive/hortator's GPL-3.0
 * firmware/src/smc.c.
 *
 * No radio dependency: the BLE transport sends these MIDI messages
 * after the connection is subscribed. The first refresh transmits
 * all 35 notes; later refreshes only changed notes.
 */
#ifndef SLOOP_SMC_LEDS_H
#define SLOOP_SMC_LEDS_H
#include <stdint.h>
#define SMCBT_LED_BITS 35u
typedef struct {
    uint8_t mute, solo, rec;   /* bit per strip 0..7 */
    uint8_t selected;          /* 0..7 */
    uint8_t playing;
    uint8_t recording;
} smcbt_led_status_t;
typedef struct {
    uint8_t known[SMCBT_LED_BITS];
    uint8_t last[SMCBT_LED_BITS];
} smcbt_led_state_t;
typedef struct {
    uint8_t status, note, velocity;
} smcbt_led_msg_t;
static void smcbt_led_reset(smcbt_led_state_t *s)
{
    unsigned i;
    for (i = 0; i < SMCBT_LED_BITS; i++)
        s->known[i] = s->last[i] = 0;
}
/* Index 0..31 = REC, SOLO, MUTE, SELECT strip LED.
 * Index 32=STOP 33=PLAY 34=REC transport LEDs.
 */
static uint8_t smcbt_led_note(unsigned idx)
{
    return (uint8_t)(idx < 32u ? idx : 0x5du + idx - 32u);
}
static uint8_t smcbt_led_want(const smcbt_led_status_t *v, unsigned idx)
{
    unsigned k = idx & 7u, mask = 1u << k;
    if (idx < 8u) return (uint8_t)((v->rec & mask) != 0);
    if (idx < 16u) return (uint8_t)((v->solo & mask) != 0);
    if (idx < 24u) return (uint8_t)((v->mute & mask) != 0);
    if (idx < 32u) return (uint8_t)(v->selected == k);
    if (idx == 32u) return (uint8_t)!v->playing;
    if (idx == 33u) return (uint8_t)!!v->playing;
    return (uint8_t)!!v->recording;
}
/* Return one changed message. The BLE sender confirms it by calling
 * smcbt_led_sent() ONLY after successfully enqueueing that message.
 */
static int smcbt_led_next(const smcbt_led_state_t *s,
                          const smcbt_led_status_t *v,
                          smcbt_led_msg_t *out, unsigned *index)
{
    unsigned i;
    for (i = 0; i < SMCBT_LED_BITS; i++) {
        uint8_t want = smcbt_led_want(v, i);
        if (s->known[i] && s->last[i] == want) continue;
        *index = i;
        out->status = 0x90u;
        out->note = smcbt_led_note(i);
        out->velocity = want ? 127u : 0u;
        return 1;
    }
    return 0;
}
static void smcbt_led_sent(smcbt_led_state_t *s, unsigned index,
                           const smcbt_led_msg_t *message)
{
    if (index >= SMCBT_LED_BITS) return;
    s->last[index] = (uint8_t)(message->velocity != 0);
    s->known[index] = 1u;
}
#endif
