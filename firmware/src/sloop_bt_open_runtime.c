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
static struct ble_rf_trims sloop_bt_hw_trims;

/* Read-only flash hook provided by Felucca's flash driver, defined later
 * in felucca.c. It reads at most 32 bytes per VM validation chunk.
 * Never program/erase calibration: SLOOP may reuse the historical VM area.
 */
static int st_read(uint32_t off, void *dst, uint32_t n);
static int sloop_bt_hw_vm_read(void *ctx,uint32_t off,uint8_t *dst,uint32_t n)
{
    (void)ctx;
    if(n>32u || off<0x93000u || off>0x0fffffu || n>0x100000u-off)return -1;
    return st_read(off,dst,n);
}
static uint8_t sloop_bt_hw_rf_validate(void)
{
    struct ble_vm_info info;
    /* A valid *complete* RF record set and x187 internal CRC are mandatory.
     * Never fall back to made-up RF trims, and never save to the VM region.
     */
    return (uint8_t)(ble_vm_scan(sloop_bt_hw_vm_read,0,&info,
                                &sloop_bt_hw_trims,0,0)==1);
}
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
    if(!sloop_bt_hw_rf_validate()) {
        sloop_bt_link.rf_cal_state=2u;
        sloop_bt_link.error=SLOOP_BT_ERR_NO_RF_CAL;
        sloop_bt_link_failed(&sloop_bt_link);
        return; /* fail closed: NO RF hardware access without verified trims */
    }
    sloop_bt_link.rf_cal_state=1u;
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
    case SLOOP_BT_CONNECT: {
        /* Only a user-selected, actually discovered SMC-Mixer may be
         * connected. No blind MAC guesses or synthetic advertisements.
         * Never write flash or alter the unit's RF calibration.
         */
        uint8_t valid=0u;
        uint32_t k;
        if(!sloop_bt_hw_started || !sloop_bt_link.enabled)break;
        for(k=0u;k<sloop_bt_link.count;k++) {
            const sloop_bt_device_t *d=&sloop_bt_link.devices[k];
            static const char smc[]="SMC-Mixer";
            uint32_t j=0u;
            if(d->address_type!=sloop_bt_link.target_type ||
               !sloop_bt_eq_addr(d->address,sloop_bt_link.target_addr))
                continue;
            while(j<sizeof smc-1u && d->name[j]==smc[j])j++;
            if(j==sizeof smc-1u && !d->name[j])valid=1u;
            break;
        }
        if(!valid) {
            sloop_bt_link.error=SLOOP_BT_ERR_GENERIC;
            sloop_bt_link_failed(&sloop_bt_link);
            break;
        }
        ble_zero((uint8_t *)&peer,sizeof peer);
        ble_cpy(peer.addr,sloop_bt_link.target_addr,6);
        peer.addr_rand=sloop_bt_link.target_type;
        fm1_ble_irqs_hold(1);
        if(!ble_central_connect(&peer)) {
            sloop_bt_link.error=SLOOP_BT_ERR_GENERIC;
            sloop_bt_link_failed(&sloop_bt_link);
        }
        fm1_ble_irqs_hold(0);
        break;
    }
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
        /* Snapshot once per second, never redraw the LCD per advertisement. */
        sloop_bt_link.scan_reports=sloop_bt_hw_scan.reports;
        sloop_bt_link.scan_kept=sloop_bt_hw_scan.kept;
        sloop_bt_link.scan_bad=sloop_bt_hw_scan.bad_ad;
        sloop_bt_link.scan_ignored=sloop_bt_hw_scan.ignored_type;
        sloop_bt_link.scan_events=drv.scan_evts;
        /* Never treat the persistent, uninitialised crash breadcrumb
         * as a per-boot interrupt counter. */
        sloop_bt_link.scan_irqs=sloop_bt_diag_rx_irqs;
        sloop_bt_link.scan_event_irqs=sloop_bt_diag_event_irqs;
        sloop_bt_link.scan_poll_hits=sloop_bt_diag_poll_hits;
        sloop_bt_link.rf_scan_found=fm1_ble_rf_stat.scan_found;
        sloop_bt_link.rf_scan_band=fm1_ble_rf_stat.scan_band;
        sloop_bt_link.rf_bad_op=fm1_ble_rf_stat.bad_op;
        sloop_bt_link.rf_bbp_timeouts=(uint16_t)fm1_ble_rf_stat.bbp_timeouts;
        sloop_bt_link.rf_spi_timeouts=(uint16_t)fm1_ble_rf_stat.spi_timeouts;
        sloop_bt_link.scan_hw_ien=(uint8_t)((FM1_BLE_IEN >> 8) & 1u);
        sloop_bt_link.scan_hw_pending=(uint8_t)((FM1_BLE_IPND >> 8) & 1u);
    }
    if(sloop_bt_link.state==SLOOP_BT_SCANNING) {
        n=ble_scan_discovery_list(&sloop_bt_hw_scan,indexes);
        for(i=0;i<n;i++){
            const struct ble_found *d=&sloop_bt_hw_scan.e[indexes[i]];
            /* Show named BLE advertisements even without the MIDI UUID; do not auto-connect. */
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
