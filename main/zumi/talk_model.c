#include "talk_model.h"
#include <string.h>
#include <stddef.h>
static bool active(const t_model *m) {
    return m->state==T_LISTENING||m->state==T_PROCESSING||m->state==T_SPEAKING;
}
static void clear_text(t_model *m) {
    memset(m->asr,0,sizeof(m->asr)); memset(m->reply,0,sizeof(m->reply));
}
static void enter(t_model *m,int state) {
    m->state=state;m->elapsed=m->silence=0;
    m->mouth_open=false;m->mouth_age=0;
    m->mic=state==T_LISTENING;m->speaker=state==T_SPEAKING;
}
static void abort_turn(t_model *m,int state,int error) {
    m->turn++;m->effect=T_F_ABORT;m->connecting=false;m->error=error;
    clear_text(m);enter(m,state);
}
/* Input adapter must validate UTF-8; keep complete codepoints at display cap. */
static void copy_text(char *out,size_t cap,const char *in) {
    if(!in) in="";
    size_t n=0;while(n<cap&&in[n])n++;
    if(n==cap) { n=cap-1;while(n&&((unsigned char)in[n]&0xc0)==0x80)n--; }
    memcpy(out,in,n);out[n]=0;
}
void t_init(t_model *m) { *m=(t_model){.state=T_OFFLINE}; }
void t_event(t_model *m,int event,uint32_t turn,const char *text) {
    m->effect=T_F_NONE;
    if(event<0||event>=T_EVENT_COUNT) return;
    if(event>=T_ASR&&turn!=m->turn) return;
    if(event==T_CANCEL) {abort_turn(m,m->online?T_IDLE:T_OFFLINE,T_E_NONE);return;}
    if(event==T_DISCONNECTED) {
        m->online=false;if(m->state==T_ERROR){return;}abort_turn(m,T_OFFLINE,T_E_NETWORK);return;
    }
    if(event==T_CONNECTED) {
        m->online=true;m->connecting=false;
        if(m->state==T_OFFLINE) {m->error=T_E_NONE;enter(m,T_IDLE);}
        return;
    }
    if(event==T_OK) {
        if(m->state==T_OFFLINE) {
            if(!m->connecting){m->connecting=true;m->elapsed=0;m->effect=T_F_CONNECT;}
        } else if(m->state==T_ERROR) {
            /* Retry prepares a fresh turn; it never resends old audio. */
            m->error=T_E_NONE;clear_text(m);enter(m,m->online?T_IDLE:T_OFFLINE);
        } else if(m->state==T_IDLE) {
            if(!m->online){enter(m,T_OFFLINE);return;}
            m->turn++;m->error=T_E_NONE;clear_text(m);
            enter(m,T_LISTENING);m->effect=T_F_BEGIN;
        } else if(m->state==T_LISTENING) {
            enter(m,T_PROCESSING);m->effect=T_F_COMMIT;
        } else abort_turn(m,T_IDLE,T_E_NONE);
        return;
    }
    if(m->connecting&&(event==T_FAILURE||event==T_OVERFLOW)) {
        abort_turn(m,T_ERROR,event==T_OVERFLOW?T_E_OVERFLOW:T_E_SERVICE);return;
    }
    if(!active(m)) return;
    if(event==T_FAILURE||event==T_OVERFLOW) {
        abort_turn(m,T_ERROR,event==T_OVERFLOW?T_E_OVERFLOW:T_E_SERVICE);return;
    }
    if(event==T_ASR&&(m->state==T_LISTENING||m->state==T_PROCESSING))
        copy_text(m->asr,sizeof(m->asr),text);
    if(event==T_REPLY&&(m->state==T_PROCESSING||m->state==T_SPEAKING))
        copy_text(m->reply,sizeof(m->reply),text); /* Bounded rolling snapshot. */
    if(event==T_AUDIO&&(m->state==T_PROCESSING||m->state==T_SPEAKING)) {
        if(m->state==T_PROCESSING) enter(m,T_SPEAKING);
        m->silence=0;
    }
    /* DONE means server end AND local playback queue drained, not server end alone. */
    if(event==T_DONE&&(m->state==T_PROCESSING||m->state==T_SPEAKING)) enter(m,T_IDLE);
}
static uint32_t add_sat(uint32_t n,uint32_t ms) {return ms>UINT32_MAX-n?UINT32_MAX:n+ms;}
void t_output_level(t_model *m,uint32_t turn,unsigned level) {
    if(turn!=m->turn||m->state!=T_SPEAKING)return;
    m->mouth_age=0;
    if(level>=450)m->mouth_open=true;
    else if(level<=220)m->mouth_open=false;
}
void t_tick(t_model *m,uint32_t ms) {
    m->mouth_age=add_sat(m->mouth_age,ms);
    if(m->mouth_age>=160)m->mouth_open=false;
    m->effect=T_F_NONE;
    m->elapsed=add_sat(m->elapsed,ms);m->silence=add_sat(m->silence,ms);
    if(m->connecting&&m->elapsed>=T_CONNECT_MS) {
        abort_turn(m,T_OFFLINE,T_E_TIMEOUT);return;
    }
    if(m->state==T_LISTENING&&m->elapsed>=T_RECORD_MS) {
        enter(m,T_PROCESSING);m->effect=T_F_COMMIT;
    } else if((m->state==T_PROCESSING&&m->elapsed>=T_PROCESS_MS)||
              (m->state==T_SPEAKING&&(m->elapsed>=T_SPEAK_MS||m->silence>=T_AUDIO_GAP_MS)))
        abort_turn(m,T_ERROR,T_E_TIMEOUT);
}
