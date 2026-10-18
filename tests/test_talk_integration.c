#include "core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void tick(z_state *s,unsigned ms){while(ms){unsigned n=ms>100?100:ms;z_tick(s,n);ms-=n;}}
int main(void){
    z_state s;z_init(&s,1);z_action(&s,Z_OK);
    assert(s.page==Z_TALK&&s.talk.state==T_OFFLINE&&!s.talk.mic);
    assert(s.talk.connecting&&z_talk_take_command(&s)==T_F_CONNECT);
    z_talk_receive(&s,T_FAILURE,s.talk.turn,NULL);
    assert(s.talk.state==T_ERROR&&!s.talk.connecting&&z_talk_take_command(&s)==T_F_ABORT);
    z_talk_receive(&s,T_DISCONNECTED,s.talk.turn,NULL);assert(!s.talk.online);
    z_action(&s,Z_OK);assert(s.talk.state==T_OFFLINE);
    z_action(&s,Z_OK);assert(z_talk_take_command(&s)==T_F_CONNECT);
    z_talk_receive(&s,T_CONNECTED,0,NULL);assert(!s.talk.mic);
    z_action(&s,Z_OK);assert(z_talk_take_command(&s)==T_F_BEGIN);
    s.idle_ms=59990;tick(&s,100);assert(!s.screen_off&&s.idle_ms==0);
    tick(&s,14900);assert(s.talk.state==T_PROCESSING&&z_talk_take_command(&s)==T_F_COMMIT);
    tick(&s,20000);assert(s.talk.state==T_ERROR&&!s.talk.mic);
    assert(z_talk_take_command(&s)==T_F_ABORT);
    unsigned old=s.talk.turn;z_action(&s,Z_BACK);
    z_talk_receive(&s,T_AUDIO,old,NULL);assert(s.page==Z_HOME&&!s.talk.speaker);
    assert(z_talk_take_command(&s)==T_F_ABORT);
    z_action(&s,Z_OK);assert(z_talk_take_command(&s)==T_F_CONNECT);
    z_talk_receive(&s,T_CONNECTED,0,NULL);tick(&s,60000);assert(s.screen_off);
    assert(z_talk_take_command(&s)==T_F_ABORT);
    tick(&s,1000);assert(z_talk_take_command(&s)==T_F_NONE); /* Stop only once. */
    z_input input={0};z_button(&s,&input,Z_OK,0);z_button(&s,&input,Z_OK,4);
    tick(&s,3000);assert(s.page==Z_HOME);
    z_action(&s,Z_DOWN);z_action(&s,Z_OK);z_action(&s,Z_OK);z_action(&s,Z_OK);
    z_state saved=s;tick(&s,60000);assert(s.screen_off);
    z_button(&s,&input,Z_OK,0);z_button(&s,&input,Z_OK,4);tick(&s,3000);
    assert(s.page==Z_LINK&&s.moves==saved.moves&&s.cursor==saved.cursor&&s.level==saved.level);
    assert(!memcmp(s.tiles,saved.tiles,sizeof(s.tiles)));
    puts("Talk integration: PASS (connect failure, retry, idle lease, timeouts, late events, game resume)");
}
