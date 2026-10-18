#include "talk_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void send(t_model *m,int ev){t_event(m,ev,m->turn,0);}
static void ready(t_model *m){t_init(m);send(m,T_CONNECTED);assert(m->state==T_IDLE);}
int main(void){
    t_model m; t_init(&m);assert(!m.mic&&!m.speaker&&m.state==T_OFFLINE);
    send(&m,T_OK);assert(m.connecting&&!m.mic&&m.effect==T_F_CONNECT);
    t_tick(&m,9999);assert(m.connecting);t_tick(&m,1);assert(!m.connecting&&m.error==T_E_TIMEOUT);
    send(&m,T_CONNECTED);assert(m.state==T_IDLE&&!m.mic);
    send(&m,T_OK);assert(m.state==T_LISTENING&&m.mic&&m.effect==T_F_BEGIN);
    unsigned old=m.turn;
    t_event(&m,T_AUDIO,m.turn,0);assert(!m.speaker); /* Never play while recording. */
    t_event(&m,T_ASR,m.turn,"What is solar power?");assert(m.asr[0]);
    send(&m,T_OK);assert(m.state==T_PROCESSING&&!m.mic&&m.effect==T_F_COMMIT);
    t_event(&m,T_REPLY,m.turn,"Energy from sunlight.");assert(m.reply[0]);
    send(&m,T_AUDIO);assert(m.state==T_SPEAKING&&m.speaker&&!m.mic);
    send(&m,T_OK);assert(m.state==T_IDLE&&!m.speaker&&!m.asr[0]&&!m.reply[0]);
    t_event(&m,T_AUDIO,old,0);assert(m.state==T_IDLE);
    send(&m,T_OK);t_event(&m,T_FAILURE,old,0);assert(m.state==T_LISTENING);
    t_tick(&m,14999);assert(m.mic);t_tick(&m,1);assert(m.state==T_PROCESSING&&!m.mic);
    t_tick(&m,19999);assert(m.state==T_PROCESSING);t_tick(&m,1);assert(m.state==T_ERROR);
    send(&m,T_OK);assert(m.state==T_IDLE&&!m.mic); /* Retry never captures. */
    for(int phase=0;phase<3;phase++){
        ready(&m);send(&m,T_OK);if(phase>0)send(&m,T_OK);if(phase>1)send(&m,T_AUDIO);
        old=m.turn;send(&m,T_DISCONNECTED);
        assert(m.state==T_OFFLINE&&!m.mic&&!m.speaker&&m.effect==T_F_ABORT);
        send(&m,T_CONNECTED);t_event(&m,T_AUDIO,old,0);assert(m.state==T_IDLE&&!m.mic);
    }
    ready(&m);send(&m,T_OK);send(&m,T_OK);send(&m,T_AUDIO);
    t_tick(&m,4999);assert(m.speaker);t_tick(&m,1);assert(m.state==T_ERROR&&!m.speaker);
    ready(&m);send(&m,T_OK);send(&m,T_OK);send(&m,T_AUDIO);
    for(int i=0;i<59;i++){t_tick(&m,1000);send(&m,T_AUDIO);}t_tick(&m,1000);
    assert(m.state==T_ERROR); /* Audio packets cannot defeat absolute timeout. */
    ready(&m);send(&m,T_OK);send(&m,T_OVERFLOW);assert(m.state==T_ERROR&&!m.mic);
    ready(&m);send(&m,T_OK);send(&m,T_OK);send(&m,T_DONE);assert(m.state==T_IDLE);
    char huge[4096];memset(huge,'x',sizeof(huge)-1);huge[sizeof(huge)-1]=0;
    send(&m,T_OK);t_event(&m,T_ASR,m.turn,huge);assert(strlen(m.asr)==T_ASR_CAP-1);
    for(int i=0;i<4092;i+=3){huge[i]=(char)0xe4;huge[i+1]=(char)0xb8;huge[i+2]=(char)0xad;}huge[4092]=0;
    send(&m,T_OK);t_event(&m,T_REPLY,m.turn,huge);assert(strlen(m.reply)%3==0);
    send(&m,T_CANCEL);assert(!m.reply[0]&&!m.asr[0]);
    unsigned rng=101;
    for(int i=0;i<100000;i++){
        rng=rng*1664525u+1013904223u;
        if(rng&1)t_event(&m,(int)(rng%T_EVENT_COUNT),(rng&2)?m.turn:m.turn-1,"sample");
        else t_tick(&m,rng%70000);
        assert(m.state>=T_IDLE&&m.state<=T_ERROR);
        assert(m.mic==(m.state==T_LISTENING));assert(m.speaker==(m.state==T_SPEAKING));
        assert(!m.mic||m.online);assert(!(m.mic&&m.speaker));
    }
    printf("Talk model: PASS (six states, timeout, cancel, stale events, UTF-8 cap, 100000 transitions; %zu bytes)\n",sizeof(m));
}
