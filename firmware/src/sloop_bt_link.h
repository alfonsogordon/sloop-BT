/* SPDX-License-Identifier: GPL-3.0-only
 * SLOOP-BT user-facing BLE link model, independent of the JieLi SDK.
 *
 * The eventual Hortator-based central driver calls the event methods
 * from SLOOP's main loop and executes the returned intent. This module
 * never starts the radio itself, never blocks and defaults OFF.
 * Initial implementation supports ONE controller only.
 */
#ifndef SLOOP_BT_LINK_H
#define SLOOP_BT_LINK_H
#include <stdint.h>
#define SLOOP_BT_DEVICES 8u
enum sloop_bt_state {
    SLOOP_BT_OFF, SLOOP_BT_IDLE, SLOOP_BT_SCANNING,
    SLOOP_BT_CONNECTING, SLOOP_BT_CONNECTED,
    SLOOP_BT_LOST, SLOOP_BT_ERROR
};
enum sloop_bt_intent {
    SLOOP_BT_DO_NOTHING, SLOOP_BT_START_RADIO, SLOOP_BT_STOP_RADIO,
    SLOOP_BT_START_SCAN, SLOOP_BT_STOP_SCAN,
    SLOOP_BT_CONNECT, SLOOP_BT_DISCONNECT
};
typedef struct {
    uint8_t address[6], address_type;
    int8_t rssi;
    char name[24];
} sloop_bt_device_t;
typedef struct {
    uint8_t state, enabled, auto_reconnect, pending;
    uint8_t count, selected, remembered, attempts;
    uint8_t target_addr[6], target_type;
    uint32_t due_ms;
    sloop_bt_device_t devices[SLOOP_BT_DEVICES];
} sloop_bt_link_t;
static inline int sloop_bt_eq_addr(const uint8_t *a,const uint8_t *b)
{
    unsigned i;
    for(i=0;i<6u;i++)if(a[i]!=b[i])return 0;
    return 1;
}
static inline void sloop_bt_link_reset(sloop_bt_link_t *m)
{
    unsigned i,j;
    uint8_t *p=(uint8_t *)m;
    for(i=0;i<sizeof *m;i++)p[i]=0;
    m->state=SLOOP_BT_OFF;
    m->auto_reconnect=1u;
    for(i=0;i<SLOOP_BT_DEVICES;i++)
        for(j=0;j<24u;j++)m->devices[i].name[j]=0;
}
static inline uint8_t sloop_bt_link_take(sloop_bt_link_t *m)
{
    uint8_t v=m->pending;
    m->pending=SLOOP_BT_DO_NOTHING;
    return v;
}
static inline void sloop_bt_link_power(sloop_bt_link_t *m,int on)
{
    if(on) {
        if(m->enabled)return;
        m->enabled=1;
        m->state=SLOOP_BT_IDLE;
        m->pending=SLOOP_BT_START_RADIO;
        m->attempts=0;
    } else {
        m->enabled=0;
        m->pending=SLOOP_BT_STOP_RADIO;
        m->state=SLOOP_BT_OFF;
        m->count=0;
        m->attempts=0;
    }
}
/* Call once the radio has completed startup, not merely after asking. */
static inline void sloop_bt_link_radio_ready(sloop_bt_link_t *m)
{
    if(m->enabled) {
        m->state=SLOOP_BT_SCANNING;
        m->pending=SLOOP_BT_START_SCAN;
        m->count=0;
    }
}
static inline void sloop_bt_link_scan(sloop_bt_link_t *m)
{
    if(!m->enabled || m->state==SLOOP_BT_OFF)return;
    if(m->state==SLOOP_BT_CONNECTED || m->state==SLOOP_BT_CONNECTING)return;
    m->state=SLOOP_BT_SCANNING;
    m->count=0;
    m->selected=0;
    m->pending=SLOOP_BT_START_SCAN;
}
static inline void sloop_bt_link_found(sloop_bt_link_t *m,const uint8_t addr[6],
                                      uint8_t type,int8_t rssi,const char *name)
{
    unsigned i,j;
    sloop_bt_device_t *d;
    if(m->state!=SLOOP_BT_SCANNING || !name)return;
    for(i=0;i<m->count;i++)
        if(m->devices[i].address_type==type &&
           sloop_bt_eq_addr(m->devices[i].address,addr))break;
    if(i==m->count) {
        if(i>=SLOOP_BT_DEVICES)return;
        m->count++;
    }
    d=&m->devices[i];
    for(j=0;j<6u;j++)d->address[j]=addr[j];
    d->address_type=type;d->rssi=rssi;
    for(j=0;j<23u && name[j];j++)d->name[j]=name[j];
    d->name[j]=0;
}
static inline void sloop_bt_link_select(sloop_bt_link_t *m,unsigned idx)
{
    sloop_bt_device_t *d;
    unsigned i;
    if(idx>=m->count || m->state!=SLOOP_BT_SCANNING)return;
    d=&m->devices[idx];
    for(i=0;i<6u;i++)m->target_addr[i]=d->address[i];
    m->target_type=d->address_type;
    m->remembered=1u;
    m->selected=(uint8_t)idx;
    m->state=SLOOP_BT_CONNECTING;
    m->pending=SLOOP_BT_CONNECT;
}
static inline void sloop_bt_link_connected(sloop_bt_link_t *m)
{
    if(m->enabled && m->state==SLOOP_BT_CONNECTING) {
        m->state=SLOOP_BT_CONNECTED;
        m->attempts=0;
        m->pending=SLOOP_BT_DO_NOTHING;
    }
}
/* Max 16 seconds between automatic retries: avoid scan load on audio. */
static inline void sloop_bt_link_lost(sloop_bt_link_t *m,uint32_t now_ms)
{
    uint32_t wait_ms;
    if(!m->enabled)return;
    m->state=SLOOP_BT_LOST;
    m->pending=SLOOP_BT_DO_NOTHING;
    if(m->attempts<4u)m->attempts++;
    wait_ms=(uint32_t)1000u<<m->attempts;
    m->due_ms=now_ms+wait_ms;
}
static inline void sloop_bt_link_tick(sloop_bt_link_t *m,uint32_t now_ms)
{
    if(!m->enabled || !m->auto_reconnect || !m->remembered ||
       m->state!=SLOOP_BT_LOST)return;
    if((int32_t)(now_ms-m->due_ms)<0)return;
    m->state=SLOOP_BT_SCANNING;
    m->pending=SLOOP_BT_START_SCAN;
    m->count=0;
}
static inline void sloop_bt_link_failed(sloop_bt_link_t *m)
{
    m->state=SLOOP_BT_ERROR;
    m->pending=SLOOP_BT_DO_NOTHING;
}
/* UI label, no formatting buffers or dynamic allocations. */
static inline const char *sloop_bt_link_status(const sloop_bt_link_t *m)
{
    static const char *const states[]={
        "OFF","READY","SCANNING","CONNECTING","CONNECTED","LOST","FAILED"
    };
    return m->state<=SLOOP_BT_ERROR?states[m->state]:"FAILED";
}
#endif
