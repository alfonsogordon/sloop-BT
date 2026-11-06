/* SPDX-License-Identifier: GPL-3.0-only
 * Standalone SMC-Mixer decoder tests. Run:
 *   cc -std=c99 -Wall -Wextra -Werror tests/smcbt_test.c -o /tmp/smcbt_test
 *   /tmp/smcbt_test
 */
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/smcbt.h"

static int fader(smcbt_state_t *s, unsigned ch, unsigned level, smcbt_event_t *e)
{
    return smcbt_midi(s, (uint8_t)(0xe0u | ch), 0u, (uint8_t)level, e);
}
static int press(smcbt_state_t *s, unsigned n, smcbt_event_t *e)
{
    return smcbt_midi(s, 0x90u, (uint8_t)n, 127u, e);
}
int main(void)
{
    smcbt_state_t s;
    smcbt_event_t e;
    const uint8_t levels[4] = {80, 40, 70, 100};
    smcbt_reset(&s, levels);
    assert(!fader(&s, 0, 10, &e));                  /* do not jump */
    assert(!fader(&s, 0, 60, &e));                  /* still below */
    assert(fader(&s, 0, 82, &e));                   /* crossed */
    assert(e.kind == SMCBT_LEVEL && e.track == 0 && e.value == 82);
    assert(fader(&s, 0, 100, &e));                  /* now engaged */
    smcbt_level_changed(&s, 0, 20);                /* remote level change */
    assert(!fader(&s, 0, 90, &e));                  /* disengaged */
    assert(fader(&s, 0, 18, &e));                   /* crossed downward */
    assert(!fader(&s, 8, 90, &e));                  /* master ignored */
    assert(!fader(&s, 4, 90, &e));                  /* absent strips ignored */

    assert(press(&s, 0x10u, &e) && e.kind == SMCBT_MUTE && e.track == 0);
    assert(press(&s, 0x1bu, &e) && e.kind == SMCBT_SELECT && e.track == 3);
    assert(!press(&s, 0x1cu, &e));                  /* track 5 not present */
    assert(press(&s, 0x5eu, &e) && e.kind == SMCBT_PLAY);
    assert(press(&s, 0x5du, &e) && e.kind == SMCBT_STOP);
    assert(!smcbt_midi(&s, 0x90u, 0x5eu, 0, &e));  /* release is not press */
    assert(smcbt_midi(&s, 0xb0u, 16u, 65u, &e));
    assert(e.kind == SMCBT_KNOB && e.index == 0 && e.delta == -1);
    assert(smcbt_midi(&s, 0xb0u, 23u, 2u, &e));
    assert(e.kind == SMCBT_KNOB && e.index == 7 && e.delta == 2);
    puts("SMC-Mixer MIDI decoder tests passed");
    return 0;
}
