/* SPDX-License-Identifier: GPL-3.0-only
 * cc -std=c99 -Wall -Wextra -Werror tests/smcbt_leds_test.c -o /tmp/smcbt_leds_test
 * /tmp/smcbt_leds_test
 */
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/smcbt_leds.h"
int main(void)
{
    smcbt_led_state_t leds;
    smcbt_led_status_t state = {0, 0, 0, 0, 0, 0};
    smcbt_led_msg_t msg;
    unsigned index, sent = 0;
    smcbt_led_reset(&leds);
    while (smcbt_led_next(&leds, &state, &msg, &index)) {
        assert(msg.status == 0x90u);
        if (index == 24u) assert(msg.note == 24u && msg.velocity == 127u);
        if (index == 32u) assert(msg.note == 0x5du && msg.velocity == 127u);
        smcbt_led_sent(&leds, index, &msg);
        assert(++sent <= SMCBT_LED_BITS);
    }
    assert(sent == SMCBT_LED_BITS);
    assert(!smcbt_led_next(&leds, &state, &msg, &index));
    state.playing = 1;
    state.mute = (uint8_t)(1u << 6);  /* drum lane C muted */
    state.solo = (uint8_t)(1u << 4);
    state.rec = (uint8_t)(1u << 5);
    state.selected = 7;
    state.recording = 1;
    assert(smcbt_led_next(&leds, &state, &msg, &index));
    assert(index == 5u && msg.note == 5u && msg.velocity == 127u);
    /* The pending message must be retried if the transport is full. */
    {
        smcbt_led_msg_t same;
        unsigned again;
        assert(smcbt_led_next(&leds, &state, &same, &again));
        assert(again == index && same.note == msg.note);
    }
    sent = 0;
    while (smcbt_led_next(&leds, &state, &msg, &index)) {
        smcbt_led_sent(&leds, index, &msg);
        assert(++sent <= SMCBT_LED_BITS);
    }
    assert(sent == 8u); /* REC strip, SOLO strip, MUTE strip, two SELECT, three transport */
    assert(!smcbt_led_next(&leds, &state, &msg, &index));
    puts("SMC-Mixer LED feedback tests passed");
    return 0;
}
