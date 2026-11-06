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
/* Legacy Hortator decoder shares a generic header guard with Optimist. */
#undef BLE_MIDI_H
#ifndef SLOOP_BT_RADIO_MEASURE
#define SLOOP_BT_RADIO_MEASURE 0
#endif
#if !SLOOP_BT_RADIO_MEASURE
#define FELUCCA_BLE_STUB 1 /* fake radio; protocol footprint only */
#endif
#include "../../third_party/optimist_ble/ble_stack.c"
#if SLOOP_BT_RADIO_MEASURE
#include "../../third_party/optimist_ble/ble_vm.c" /* read-only RF calibration VM */
#endif
#include "sloop_bt_midi_ring.h"
static sloop_bt_midi_ring_t sloop_bt_open_rx;

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
    (void)sloop_bt_midi_ring_push(&sloop_bt_open_rx,pkt);
}
BLE_API void ble_app_state(void){sloop_bt_open_state_dirty=1;}

/* Force retention of the central, scan, MIDI and outgoing feedback
 * paths for measurement, but never call this from the product build.
 * The blob is internal to the measurement build and not released.
 */
/* Volatile function pointers prevent whole-program optimisation of the
 * fake-radio probes into empty operations. A real BLE link may invoke
 * these paths asynchronously; the fake baseband cannot model that.
 */
static void (* volatile measured_init)(const uint8_t *,uint8_t)=ble_init;
static void (* volatile measured_enable)(int)=ble_enable;
static void (* volatile measured_scan)(int)=ble_ll_scan;
static int (* volatile measured_connect)(const struct ble_peer *)=ble_central_connect;
static void (* volatile measured_cancel)(void)=ble_central_cancel;
static int (* volatile measured_ready)(void)=ble_midi_ready;
static uint8_t (* volatile measured_state)(void)=ble_central_state;
static uint8_t (* volatile measured_fail)(void)=ble_central_fail;
static __attribute__((noinline,used)) int sloop_bt_open_probe(void)
{
    static const uint8_t addr[6]={0x12,0x34,0x56,0x78,0x9a,0xc0};
    struct ble_peer peer;
    ble_zero((uint8_t *)&peer,sizeof peer);
    uint8_t packet[8];
    unsigned index=0;
    uint32_t n;
    measured_init(addr,1);
    measured_enable(1);
    measured_scan(1);
    peer.addr_rand=1;
    if(measured_connect(&peer))measured_cancel();
    n=sloop_bt_led_packet(0u,packet,&index);
    if(n) {
        sloop_bt_open_out_pkt=0x0990007fu;
        sloop_bt_open_out_time=0u;
        sloop_bt_open_out_pending=1;
        return (int)n+(int)measured_state()+(int)measured_ready();
    }
    return (int)ble_connected()+(int)measured_fail();
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
    uint32_t packet, i;
    if(!ran) {
        ran=1;
        sloop_bt_midi_ring_reset(&sloop_bt_open_rx);
        sloop_bt_open_measure_sink=sloop_bt_open_probe();
    }
    for(i=0;i<16u && sloop_bt_midi_ring_pop(&sloop_bt_open_rx,&packet);i++)
        sloop_bt_midi_usb_packet(packet);
}
