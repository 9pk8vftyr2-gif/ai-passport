#include "talk_model.h"
#include <stdio.h>
int main(void){
    t_model m;t_init(&m);int event;unsigned turn,ms;
    while(scanf("%d %u %u",&event,&turn,&ms)==3){
        if(event<0)t_tick(&m,ms);else t_event(&m,event,turn,0);
        printf("%d %d %d %u %u %d %d %d\n",m.state,m.effect,m.error,m.turn,m.elapsed,m.mic,m.speaker,m.connecting);
    }
}
