/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP-BT OPEN STACK SIZE PROBE -- NONFLASHABLE RESEARCH ONLY.
 *
 * Compile Optimist's real core/central code with its harmless fake
 * hardware so we can measure the target-XIP contribution independently
 * from the FM1 radio driver. NOT an operational BLE firmware.
 *
 * No actual radio is started and this diagnostic MUST NOT be released.
 * Build profile SLOOP_BT_OPEN_MEASURE=1.
 */
#ifndef SLOOP_BT_OPEN_MEASURE
#define SLOOP_BT_OPEN_MEASURE 0
#endif
#if SLOOP_BT_OPEN_MEASURE
#if !SLOOP_BT_CONTROL
#error "Measurement requires SLOOP_BT_CONTROL"
#endif
#define BLE_CENTRAL 3      /* minimal outgoing BLE central, without pairing */
#define BLE_DIAG 0         /* diagnostics are not part of the minimal build */
#define BLE_DEVICE_NAME "SLOOP-BT"
#define BLE_NAME_HEX 0
#define FELUCCA_BLE_STUB 1 /* fake radio; protocol footprint only */
/* Legacy Hortator decoder shares a generic header guard with Optimist.
 * Both codecs are standalone and use different type/function names. */
#undef BLE_MIDI_H
#include "../../third_party/optimist_ble/ble_stack.c"

/* Strict IRQ-to-main bridge will replace these once the hardware port
 * exists. The radio stub produces no interrupts or real MIDI.
 */
static uint32_t sloop_bt_open_out_pkt, sloop_bt_open_out_time;
static uint8_t sloop_bt_open_out_pending;
static volatile uint8_t sloop_bt_open_state_dirty;

BLE_API int ble_app_midi_peek(uint32_t *pkt,uint32_t *time)
{
    if(!sloop_bt_open_out_pending)return 0;
    *pkt=sloop_bt_open_out_pkt;
    *time=sloop_bt_open_out_time;
    return 1;
}
BLE_API void ble_app_midi_pop(void) {sloop_bt_open_out_pending=0;}
BLE_API void ble_app_midi_in(uint32_t pkt,uint16_t timestamp,uint16_t last)
{
    (void)timestamp;(void)last;
    /* Diagnostic stub: fake BLE interrupts do not run. Real hardware
     * MUST use a bounded FIFO and drain this on the main loop.
     */
    sloop_bt_receive_midi((uint8_t)(pkt>>8),(uint8_t)(pkt>>16),(uint8_t)(pkt>>24));
}
BLE_API void ble_app_state(void){sloop_bt_open_state_dirty=1;}

/* Force retention of the central, scan, MIDI and outgoing feedback
 * paths for measurement, but never call this from the product build.
 * The blob is internal to the measurement build and not released.
 */
static __attribute__((noinline,used)) int sloop_bt_open_probe(void)
{
    static const uint8_t addr[6]={0x12,0x34,0x56,0x78,0x9a,0xc0};
    struct ble_peer peer;
    ble_zero((uint8_t *)&peer,sizeof peer);
    uint8_t packet[8];
    unsigned index=0;
    uint32_t n;
    ble_init(addr,1);
    ble_enable(1);
    ble_ll_scan(1);
    peer.addr_rand=1;
    if(ble_central_connect(&peer))ble_central_cancel();
    n=sloop_bt_led_packet(0u,packet,&index);
    if(n) {
        sloop_bt_open_out_pkt=0x0990007fu;
        sloop_bt_open_out_time=0u;
        sloop_bt_open_out_pending=1;
        return (int)n+(int)ble_central_state()+(int)ble_midi_ready();
    }
    return (int)ble_connected()+(int)ble_central_fail();
}
#endif

/* Called from main.c in the measurement-only variant, so interprocedural
 * DCE cannot delete the entire Bluetooth codepath as unused. Never
 * present in a packaged build (the tool refuses to package this mode).
 */
static volatile int sloop_bt_open_measure_sink;
static void sloop_bt_open_measure_tick(void)
{
    static uint8_t ran;
    if(!ran) {
        ran=1;
        sloop_bt_open_measure_sink=sloop_bt_open_probe();
    }
}
