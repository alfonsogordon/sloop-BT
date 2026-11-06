/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../firmware/src/sloop_bt_link.h"
int main(void)
{
    sloop_bt_link_t l;
    uint8_t a[6]={1,2,3,4,5,6},b[6]={7,8,9,10,11,12};
    sloop_bt_link_reset(&l);
    assert(l.state==SLOOP_BT_OFF && l.auto_reconnect);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_DO_NOTHING);
    sloop_bt_link_found(&l,a,0,-40,"SMC-Mixer");
    assert(l.count==0);
    sloop_bt_link_power(&l,1);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_START_RADIO);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_DO_NOTHING);
    sloop_bt_link_radio_ready(&l);
    assert(l.state==SLOOP_BT_SCANNING);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_START_SCAN);
    sloop_bt_link_found(&l,a,1,-55,"SMC-Mixer");
    sloop_bt_link_found(&l,a,1,-39,"SMC-Mixer");
    sloop_bt_link_found(&l,b,1,-30,"Keyboard");
    assert(l.count==2 && l.devices[0].rssi==-39);
    assert(l.devices[1].name[0]=='K');
    sloop_bt_link_select(&l,4);
    assert(l.state==SLOOP_BT_SCANNING && l.pending==0);
    sloop_bt_link_select(&l,0);
    assert(l.state==SLOOP_BT_CONNECTING && l.remembered);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_CONNECT);
    assert(sloop_bt_eq_addr(l.target_addr,a));
    sloop_bt_link_connected(&l);
    assert(l.state==SLOOP_BT_CONNECTED);
    assert(sloop_bt_link_status(&l)[0]=='C');
    sloop_bt_link_lost(&l,1000);
    assert(l.state==SLOOP_BT_LOST && l.due_ms==3000);
    sloop_bt_link_tick(&l,2999);
    assert(l.state==SLOOP_BT_LOST);
    sloop_bt_link_tick(&l,3000);
    assert(l.state==SLOOP_BT_SCANNING);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_START_SCAN);
    sloop_bt_link_found(&l,b,1,-25,"Other keyboard");
    assert(l.state==SLOOP_BT_SCANNING); /* not a saved address */
    sloop_bt_link_found(&l,a,1,-45,"SMC-Mixer");
    assert(l.state==SLOOP_BT_CONNECTING);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_CONNECT);
    sloop_bt_link_power(&l,0);
    assert(l.state==SLOOP_BT_OFF && !l.enabled);
    assert(sloop_bt_link_take(&l)==SLOOP_BT_STOP_RADIO);
    sloop_bt_link_tick(&l,999999);
    assert(l.state==SLOOP_BT_OFF);
    sloop_bt_link_power(&l,1);
    sloop_bt_link_failed(&l);
    assert(l.state==SLOOP_BT_ERROR);
    sloop_bt_link_power(&l,0);
    puts("SLOOP-BT BLE link model tests passed");
    return 0;
}
