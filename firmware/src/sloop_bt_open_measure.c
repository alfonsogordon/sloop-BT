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
#ifndef SLOOP_BT_RADIO_MEASURE
#define SLOOP_BT_RADIO_MEASURE 0
#endif
#if !SLOOP_BT_RADIO_MEASURE
#define FELUCCA_BLE_STUB 1 /* fake radio; protocol footprint only */
#endif
/* Per-boot diagnostics: unlike Optimist's crash breadcrumb in .noinit,
 * these counters start at zero and track actual radio IRQ entries. */
#if SLOOP_BT_RADIO_MEASURE
static volatile uint32_t sloop_bt_diag_rx_irqs;
static volatile uint32_t sloop_bt_diag_event_irqs;
static volatile uint32_t sloop_bt_diag_poll_hits;
#endif
#include "../../third_party/optimist_ble/ble_stack.c"
#if SLOOP_BT_RADIO_MEASURE
#include "../../third_party/optimist_ble/ble_vm.c" /* read-only RF calibration VM */
#endif
#include "sloop_bt_midi_ring.h"
static sloop_bt_midi_ring_t sloop_bt_open_rx;
#if SLOOP_BT_RADIO_MEASURE
#include "sloop_bt_open_runtime.c"
#endif

/* Strict IRQ-to-main bridge will replace these once the hardware port
 * exists. The radio stub produces no interrupts or real MIDI.
 */
static uint32_t sloop_bt_open_out_pkt, sloop_bt_open_out_time;
static volatile uint8_t sloop_bt_open_out_pending, sloop_bt_open_out_acked;
static uint8_t sloop_bt_open_out_led_index, sloop_bt_open_out_led_on;
static volatile uint8_t sloop_bt_open_state_dirty;

BLE_API int ble_app_midi_peek(uint32_t *pkt,uint32_t *time)
{
    if(!sloop_bt_open_out_pending)return 0;
    *pkt=sloop_bt_open_out_pkt;
    *time=sloop_bt_open_out_time;
    return 1;
}
BLE_API void ble_app_midi_pop(void)
{
    sloop_bt_open_out_pending=0;
    sloop_bt_open_out_acked=1; /* LED acknowledgement on main loop */
}
BLE_API void ble_app_midi_in(uint32_t pkt,uint16_t timestamp,uint16_t last)
{
    (void)timestamp;(void)last;
    /* Diagnostic stub: fake BLE interrupts do not run. Real hardware
     * MUST use a bounded FIFO and drain this on the main loop.
     */
    (void)sloop_bt_midi_ring_push(&sloop_bt_open_rx,pkt);
}
BLE_API void ble_app_state(void){sloop_bt_open_state_dirty=1;}

/* Queue one SMC LED output event in Optimist's native USB-MIDI word
 * format. The BLE codec handles BLE packet framing itself.
 * Called from main loop only; no SLOOP UI state touched in IRQ.
 */
static void sloop_bt_open_led_prepare(uint32_t now_ms)
{
    smcbt_led_status_t status;
    smcbt_led_msg_t msg;
    unsigned index;
    if(!sloop_bt_connected || sloop_bt_open_out_pending)return;
    sloop_bt_control_led_status(&status);
    if(!smcbt_led_next(&sloop_bt_leds,&status,&msg,&index))return;
    sloop_bt_open_out_pkt=9u|((uint32_t)msg.status<<8)|
                          ((uint32_t)msg.note<<16)|((uint32_t)msg.velocity<<24);
    sloop_bt_open_out_time=now_ms;
    sloop_bt_open_out_led_index=(uint8_t)index;
    sloop_bt_open_out_led_on=(uint8_t)(msg.velocity!=0u);
    SLOOP_BT_COMPILER_BARRIER();
    sloop_bt_open_out_pending=1;
}

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
static volatile int sloop_bt_open_measure_sink;
#if SLOOP_BT_RADIO_MEASURE
/* These physical interrupt handlers are essential for RX, GATT,
 * subscriptions and the MIDI path. Retain them even though SLOOP
 * has not yet installed real IRQ hooks in the vector table.
 * Their pointers enter the data image and prevent elimination.
 */
static void (* volatile measured_irq_rx)(void)=ble_wl82_rx_irq;
static void (* volatile measured_irq_evt)(void)=ble_wl82_event_irq;
/* The upstream radio application calls this explicitly before
 * enabling BLE. Include full RF startup and its calibration tables
 * in the measured image even though the probe must not execute it. */
static void (* volatile measured_rf_start)(const struct ble_rf_trims *)=ble_hw_wl82_start;
#endif
static __attribute__((noinline,used)) int sloop_bt_open_probe(void)
{
    static const uint8_t addr[6]={0x12,0x34,0x56,0x78,0x9a,0xc0};
    struct ble_peer peer;
    ble_zero((uint8_t *)&peer,sizeof peer);
    measured_init(addr,1);
    measured_enable(1);
    measured_scan(1);
    peer.addr_rand=1;
    if(measured_connect(&peer))measured_cancel();
#if SLOOP_BT_RADIO_MEASURE
    /* Load volatile IRQ pointers to keep RX/event code reachable.
     * DO NOT execute handlers without a real radio interrupt.
     */
    sloop_bt_open_measure_sink += (int)((uintptr_t)measured_irq_rx & 1u)
                                + (int)((uintptr_t)measured_irq_evt & 1u)
                                + (int)((uintptr_t)measured_rf_start & 1u);
#endif
    return (int)ble_connected()+(int)measured_fail()
          +(int)measured_state()+(int)measured_ready();
}
#endif

/* Called from main.c in the measurement-only variant, so interprocedural
 * DCE cannot delete the entire Bluetooth codepath as unused. Never
 * present in a packaged build (the tool refuses to package this mode).
 */
static void sloop_bt_open_measure_tick(void)
{
    static uint8_t ran;
    uint32_t packet, i;
    if(!ran) {
        ran=1;
        sloop_bt_midi_ring_reset(&sloop_bt_open_rx);
#if SLOOP_BT_RADIO_MEASURE
        /* Real-radio diagnostic: never start radio on boot. A user action
         * in the guarded menu must explicitly request power-on.
         * Take a volatile function address to retain probe code.
         */
        sloop_bt_open_measure_sink=(int)((uintptr_t)&sloop_bt_open_probe & 1u);
#else
        sloop_bt_open_measure_sink=sloop_bt_open_probe();
#endif
    }
#if SLOOP_BT_RADIO_MEASURE
    sloop_bt_hw_poll(fm1_ms);
#endif
    if(sloop_bt_open_state_dirty) {
        sloop_bt_open_state_dirty=0;
        if(ble_midi_ready()) {
            if(!sloop_bt_connected)sloop_bt_control_connect();
        } else if(sloop_bt_connected) {
            sloop_bt_control_disconnect();
            sloop_bt_open_out_pending=0;
        }
    }
    if(sloop_bt_open_out_acked) {
        sloop_bt_open_out_acked=0;
        sloop_bt_led_queued(sloop_bt_open_out_led_index,sloop_bt_open_out_led_on);
    }
    for(i=0;i<16u && sloop_bt_midi_ring_pop(&sloop_bt_open_rx,&packet);i++)
        sloop_bt_midi_usb_packet(packet);
    if(sloop_bt_connected) {
        sloop_bt_control_sync_levels();
        sloop_bt_open_led_prepare(fm1_ms);
    }
}
