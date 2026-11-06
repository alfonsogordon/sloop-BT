/* SPDX-License-Identifier: GPL-3.0-only
 * Optimist WL82 live-central adapter, DIAGNOSTIC-ONLY until actual FM1
 * hardware bring-up, RF calibration and audio-load tests have passed.
 *
 * Included inside sloop_bt_open_measure.c after ble_stack.c.
 * SLOOP_BT_OPEN_MEASURE=1 and SLOOP_BT_RADIO_MEASURE=1 are REQUIRED.
 * The build deliberately produces NO .fwsc firmware package.
 */
#if !SLOOP_BT_OPEN_MEASURE || !SLOOP_BT_RADIO_MEASURE
#error "Optimist WL82 runtime requires a nonflashable real-radio diagnostic"
#endif

static struct ble_scan_tab sloop_bt_hw_scan;
static uint8_t sloop_bt_hw_started;
static uint32_t sloop_bt_hw_last_age;

/* Optimist reference: device/bt_own.c OWN_TRIMS, with x187 measured by
 * the upstream RF calibration. No flash erase or VM write is performed.
 * Do not use outside a guarded bench test without unit-specific RF QA.
 */
static const struct ble_rf_trims sloop_bt_hw_trims = {
    {0x0b,0x0b}, {1,7,4,7,11,1,7}, {0}, {0}
};
static void sloop_bt_hw_start(void)
{
    /* Diagnostic only: locally-administered static random address.
     * A production port must derive and save a unit-unique address.
     */
    static const uint8_t address[6]={0x51,0x4c,0x4f,0x4f,0x50,0xcf};
    if(sloop_bt_hw_started) {
        /* The RF is already calibrated after an earlier OFF/ON.
         * Re-enable the link layer without retuning an active block.
         */
        fm1_ble_irqs_hold(1);
        ble_enable(1);
        fm1_ble_irqs_hold(0);
        sloop_bt_link_radio_ready(&sloop_bt_link);
        return;
    }
    ble_init(address,1u);
    fm1_irq_off();
    fm1_guard_unlock_top();
    fm1_ble_irq_attach(BLE_HW_IRQ_PRIO); /* RX 29 / event 45 below audio */
    fm1_ble_irqs_hold(1);
    fm1_guard_lock_top();
    fm1_irq_on();
    /* Imported V15 RF path handles its own startup and VCO scan. */
    ble_hw_wl82_start(&sloop_bt_hw_trims);
    fm1_ble_irqs_hold(1);
    ble_enable(1);
    fm1_ble_irqs_hold(0);
    sloop_bt_hw_started=1;
    ble_scan_clear(&sloop_bt_hw_scan);
    sloop_bt_link_radio_ready(&sloop_bt_link);
}
static void sloop_bt_hw_intent(void)
{
    struct ble_peer peer;
    uint8_t action=sloop_bt_link_take(&sloop_bt_link);
    if(!action)return;
    switch(action) {
    case SLOOP_BT_START_RADIO:
        sloop_bt_hw_start();
        break;
    case SLOOP_BT_STOP_RADIO:
        if(sloop_bt_hw_started) {
            fm1_ble_irqs_hold(1);
            ble_central_cancel();
            ble_ll_scan(0);
            ble_enable(0);
            fm1_ble_irqs_hold(0);
        }
        if(sloop_bt_connected)sloop_bt_control_disconnect();
        break;
    case SLOOP_BT_START_SCAN:
        if(!sloop_bt_hw_started)break;
        fm1_ble_irqs_hold(1);
        ble_scan_clear(&sloop_bt_hw_scan);
        ble_ll_scan(1);
        fm1_ble_irqs_hold(0);
        break;
    case SLOOP_BT_STOP_SCAN:
        if(sloop_bt_hw_started) {
            fm1_ble_irqs_hold(1);
            ble_ll_scan(0);
            fm1_ble_irqs_hold(0);
        }
        break;
    case SLOOP_BT_CONNECT:
        if(!sloop_bt_hw_started)break;
        ble_zero((uint8_t *)&peer,sizeof peer);
        ble_cpy(peer.addr,sloop_bt_link.target_addr,6);
        peer.addr_rand=sloop_bt_link.target_type;
        fm1_ble_irqs_hold(1);
        if(!ble_central_connect(&peer))sloop_bt_link_failed(&sloop_bt_link);
        fm1_ble_irqs_hold(0);
        break;
    case SLOOP_BT_DISCONNECT:
        if(sloop_bt_hw_started) {
            fm1_ble_irqs_hold(1);
            ble_central_cancel();
            fm1_ble_irqs_hold(0);
        }
        break;
    default: break;
    }
}
static void sloop_bt_hw_poll(uint32_t now_ms)
{
    uint8_t report[40],indexes[BLE_SCAN_N];
    uint16_t rssi;
    uint32_t i,n,count=0;
    if(!sloop_bt_link.enabled && !sloop_bt_hw_started)return;
    sloop_bt_link_tick(&sloop_bt_link,now_ms);
    sloop_bt_hw_intent();
    if(!sloop_bt_hw_started)return;
    /* Drain at most eight advertisements per UI iteration so scanning
     * cannot starve MIDI rendering or normal screen processing.
     */
    while(count++<8u && (n=ble_ll_scan_take(report,&rssi))!=0u)
        ble_scan_add(&sloop_bt_hw_scan,report,n,rssi,now_ms);
    if(now_ms-sloop_bt_hw_last_age>=1000u) {
        sloop_bt_hw_last_age=now_ms;
        ble_scan_age(&sloop_bt_hw_scan,now_ms);
    }
    if(sloop_bt_link.state==SLOOP_BT_SCANNING) {
        n=ble_scan_list(&sloop_bt_hw_scan,indexes);
        for(i=0;i<n;i++){
            const struct ble_found *d=&sloop_bt_hw_scan.e[indexes[i]];
            /* Only BLE-MIDI advertisements are returned by ble_scan_list. */
            sloop_bt_link_found(&sloop_bt_link,d->addr,d->addr_rand,
                                (int8_t)(-100+20*(int)ble_scan_bars(&sloop_bt_hw_scan,indexes[i])),
                                d->name);
        }
    }
    if(sloop_bt_link.state==SLOOP_BT_CONNECTING &&
       ble_central_state()==BLE_CS_READY && ble_midi_ready())
        sloop_bt_link_connected(&sloop_bt_link);
    else if(sloop_bt_link.state==SLOOP_BT_CONNECTING &&
            ble_central_state()==BLE_CS_IDLE && ble_central_fail()!=BLE_CF_NONE) {
        uint8_t fail=ble_central_fail();
        sloop_bt_link.error=fail==BLE_CF_NEED_PAIR ? SLOOP_BT_ERR_PAIR_REQUIRED :
                            fail==BLE_CF_NO_MIDI ? SLOOP_BT_ERR_NO_MIDI :
                            fail==BLE_CF_GATT ? SLOOP_BT_ERR_GATT :
                            fail==BLE_CF_LOST ? SLOOP_BT_ERR_LOST :
                            SLOOP_BT_ERR_GENERIC;
        sloop_bt_link_failed(&sloop_bt_link);
    }
    else if(sloop_bt_link.state==SLOOP_BT_CONNECTED && !ble_midi_ready())
        sloop_bt_link_lost(&sloop_bt_link,now_ms);
}
