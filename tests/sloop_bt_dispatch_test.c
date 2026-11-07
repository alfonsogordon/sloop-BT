/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/sloop_bt_dispatch.h"
typedef struct {unsigned kind,index,value,focus,selected,recorded,transport,knob;int delta;} trace_t;
static void lev(void *c,unsigned t,unsigned v){trace_t *x=c;x->kind=1;x->index=t;x->value=v;}
static void mut(void *c,unsigned t){trace_t *x=c;x->kind=2;x->index=t;}
static void sol(void *c,unsigned t){trace_t *x=c;x->kind=3;x->index=t;}
static void sel(void *c,unsigned t){trace_t *x=c;x->selected=t;}
static void rec(void *c,unsigned t){trace_t *x=c;x->recorded=t+1u;}
static void trn(void *c,unsigned t){trace_t *x=c;x->transport=t;}
static void knb(void *c,unsigned n,int delta){trace_t *x=c;x->knob=n+1u;x->delta=delta;}
static void focus(void *c,unsigned n){trace_t *x=c;x->focus=n+1u;}
int main(void)
{
    sloop_bt_drums_t drums;
    trace_t trace={0};
    const sloop_bt_ops_t ops={lev,mut,sol,sel,rec,trn,knb,focus};
    smcbt_event_t e={0};
    sloop_bt_drums_reset(&drums);
    e.kind=SMCBT_LEVEL;e.track=3;e.value=99;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.kind==1 && trace.index==3 && trace.value==99);
    e.kind=SMCBT_DRUM_LEVEL;e.track=4;e.value=61;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(drums.gain[0]==61 && trace.kind==1); /* no synth callback */
    e.kind=SMCBT_MUTE;e.track=5;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(drums.mute==(1u<<2) && trace.kind==1);
    e.kind=SMCBT_SOLO;e.track=5;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(drums.solo==(1u<<2));
    e.kind=SMCBT_SELECT;e.track=6;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.selected==3 && trace.focus==5); /* hat lane 4 */
    e.kind=SMCBT_REC_TRACK;e.track=7;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.recorded==4 && trace.selected==3 && trace.focus==4); /* clap lane 3 */
    e.kind=SMCBT_MUTE;e.track=1;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.kind==2 && trace.index==1);
    e.kind=SMCBT_SOLO;e.track=0;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.kind==3 && trace.index==0);
    e.kind=SMCBT_PLAY;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.transport==1);
    e.kind=SMCBT_STOP;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.transport==2);
    e.kind=SMCBT_KNOB;e.index=6;e.delta=-2;
    sloop_bt_dispatch(&e,&drums,&ops,&trace);
    assert(trace.knob==7 && trace.delta==-2);
    puts("SLOOP-BT strip dispatcher tests passed");
    return 0;
}
