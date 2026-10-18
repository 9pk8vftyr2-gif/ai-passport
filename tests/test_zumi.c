#include "core.h"
#include "view.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void start(z_state *s,int game) {
    z_init(s,42); z_action(s,Z_DOWN); z_action(s,Z_OK);
    if(game==Z_JOURNEY) z_action(s,Z_DOWN);
    z_action(s,Z_OK); assert(s->page==game);
}
static void advance(z_state *s,int ms) { for(int i=0;i<ms;i+=20) z_tick(s,20); }
static void aim(z_state *s,int lane) {
    while(s->lane>lane)z_action(s,Z_UP);
    while(s->lane<lane)z_action(s,Z_DOWN);
}
static void view_bounds(z_state *s) {
    z_view v; z_render(s,-1,&v);
    assert(v.count>0 && v.count<Z_DRAW_MAX);
    for(int i=0;i<v.count;i++) {
        z_draw *d=&v.ops[i];
        assert(d->x>=0 && d->y>=0 && d->x+d->w<=240);
        if(d->kind!=Z_TEXT) assert(d->y+d->h<=320);
        assert(memchr(d->text,0,64));
    }
}
int main(void) {
    z_state s,a,b; z_input input={0};
    z_init(&s,0); assert(s.page==Z_HOME && s.rng!=0);
    z_action(&s,Z_UP); assert(s.selection==0);
    for(int i=0;i<10;i++) z_action(&s,Z_DOWN);
    assert(s.selection==2); z_action(&s,Z_OK); assert(s.page==Z_GARDEN);
    assert(z_garden_sync(&s)==Z_SERVICE_UNAVAILABLE);
    z_action(&s,Z_OK); assert(s.page==Z_HOME);
    z_action(&s,Z_OK); assert(s.page==Z_TALK && z_talk_request()==Z_SERVICE_UNAVAILABLE);
    z_action(&s,Z_OK);
    // Long OK exits once; release and generated CLICK must not reopen Talk.
    z_button(&s,&input,Z_OK,0); z_button(&s,&input,Z_OK,3);
    z_button(&s,&input,Z_OK,4); z_button(&s,&input,Z_OK,1);
    assert(s.page==Z_HOME);
    z_button(&s,&input,Z_OK,4); assert(s.page==Z_HOME);
    z_button(&s,&input,Z_DOWN,0); z_button(&s,&input,Z_DOWN,1);
    assert(s.selection==1);
    z_button(&s,&input,Z_OK,0); z_button(&s,&input,Z_OK,4); assert(s.page==Z_PLAY);
    puts("PASS navigation, clamping, service stubs, short/long/release suppression");
    start(&s,Z_CARBON); aim(&s,s.carbon_lane); z_action(&s,Z_OK);
    assert(s.score==10 && s.lives==3);
    aim(&s,s.leaf_lane); z_action(&s,Z_OK); assert(s.lives==2 && s.score==10);
    advance(&s,1800); assert(s.lives==1);
    advance(&s,1800); assert(s.page==Z_RESULT && !s.won);
    int score=s.score;advance(&s,1000);assert(s.score==score);
    z_action(&s,Z_OK); assert(s.page==Z_CARBON && s.score==0 && s.lives==3);
    for(int i=0;i<300;i++) { if(i%10==0){aim(&s,s.carbon_lane);z_action(&s,Z_OK);} z_tick(&s,100); }
    assert(s.page==Z_RESULT && s.won && s.elapsed==30000);
    puts("PASS Carbon Pop hit, protected leaf, expiry, loss, replay, timed win");
    start(&s,Z_JOURNEY);s.items[0]=(z_item){42,1,0,1};z_tick(&s,20);assert(s.lives==2);
    s.items[0]=(z_item){42,1,0,1};z_action(&s,Z_OK);z_tick(&s,20);assert(s.lives==2);
    assert(s.jump==580);z_action(&s,Z_OK);assert(s.jump==580);
    s.items[0]=(z_item){42,1,1,1};z_tick(&s,20);assert(s.score==5);
    advance(&s,1000);assert(s.cooldown==0);
    s.items[0]=(z_item){42,0,0,1};z_tick(&s,20);assert(s.lives==2);
    s.elapsed=29980;z_tick(&s,20);assert(s.page==Z_RESULT && s.won);
    z_action(&s,Z_BACK);z_tick(&s,100);assert(s.page==Z_HOME);
    puts("PASS Journey rock, jump, cooldown, leaf, other lane, finish, exit");
    start(&a,Z_JOURNEY);b=a;
    for(int i=0;i<5;i++)z_tick(&a,20);
    z_tick(&b,100);assert(memcmp(&a,&b,sizeof(a))==0);
    z_tick(&a,1000000);assert(a.elapsed==200);
    puts("PASS fixed-step determinism and suspension cap");
    for(int seed=1;seed<=50;seed++) {
        z_init(&s,(unsigned)seed);
        unsigned r=(unsigned)seed;
        for(int i=0;i<2000;i++) {
            r=r*1664525u+1013904223u;
            if((r&7)<4)z_action(&s,(int)(r&3));else z_tick(&s,(r>>8)%200);
            assert(s.page>=Z_HOME && s.page<=Z_RESULT);
            assert(s.lane>=0 && s.lane<3 && s.lives>=0 && s.lives<=3);
            view_bounds(&s);
        }
    }
    puts("PASS 100000 randomized transitions and draw-list bounds");
    printf("State: %zu bytes; draw list: %zu bytes\n",sizeof(s),sizeof(z_view));
    return 0;
}
