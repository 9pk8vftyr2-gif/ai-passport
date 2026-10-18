#include "talk_model.h"
static t_model model;
static char incoming[T_REPLY_CAP];
void talk_init(void){t_init(&model);}
void talk_event(int event,unsigned turn){t_event(&model,event,turn,0);}
void talk_tick(unsigned ms){t_tick(&model,ms);}
int talk_state(void){return model.state;}
int talk_effect(void){return model.effect;}
int talk_error(void){return model.error;}
unsigned talk_turn(void){return model.turn;}
unsigned talk_elapsed(void){return model.elapsed;}
int talk_mic(void){return model.mic;}
int talk_speaker(void){return model.speaker;}
int talk_connecting(void){return model.connecting;}
int talk_size(void){return sizeof(model);}
char *talk_buffer(void){return incoming;}
const char *talk_asr(void){return model.asr;}
const char *talk_reply(void){return model.reply;}
void talk_text(int event,unsigned turn){t_event(&model,event,turn,incoming);}
