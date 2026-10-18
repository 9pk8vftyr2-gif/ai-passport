#include "core.h"
#include "view.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void advance(z_state *s,int ms) { while(ms>0){int dt=ms>20?20:ms;z_tick(s,dt);ms-=dt;} }
static void bounds(z_state *s) {
    z_view v;z_render(s,-1,&v);assert(v.count>0&&v.count<Z_DRAW_MAX);
    for(int i=0;i<v.count;i++) { z_draw *d=&v.ops[i];
        assert(d->x>=0&&d->y>=0&&d->x+d->w<=240&&d->y+d->h<=320);
        assert(d->opacity>=0&&d->opacity<=255);assert(memchr(d->text,0,64));
    }
}
/* Independent edge-list traversal catches wrapping and one-sided connectors. */
static unsigned oracle(const z_state *s) {
    unsigned seen=1u<<s->source,previous;
    do { previous=seen;
        for(int i=0;i<9;i++) {
            if(i%3<2 && (s->tiles[i]&2) && (s->tiles[i+1]&8) && (seen&((1u<<i)|(1u<<(i+1))))) seen|=(1u<<i)|(1u<<(i+1));
            if(i<6 && (s->tiles[i]&4) && (s->tiles[i+3]&1) && (seen&((1u<<i)|(1u<<(i+3))))) seen|=(1u<<i)|(1u<<(i+3));
        }
    } while(seen!=previous);return seen;
}
int main(void) {
    z_state s;z_input in={0};z_init(&s,42);
    z_action(&s,Z_OK);assert(s.page==Z_TALK&&z_talk_request()==-1);z_action(&s,Z_OK);
    z_action(&s,Z_DOWN);z_action(&s,Z_DOWN);z_action(&s,Z_OK);assert(s.page==Z_GARDEN&&z_garden_sync(&s)==-1);
    z_button(&s,&in,Z_OK,0);z_button(&s,&in,Z_OK,3);z_button(&s,&in,Z_OK,4);assert(s.page==Z_HOME);
    z_action(&s,Z_DOWN);z_action(&s,Z_OK);assert(s.page==Z_PLAY);z_action(&s,Z_UP);assert(s.level==5);z_action(&s,Z_DOWN);assert(s.level==0);
    for(int level=0;level<Z_LEVELS;level++) {
        z_link_start(&s,level);assert(!s.won);uint8_t fixed[9];memcpy(fixed,s.tiles,9);
        advance(&s,70000);assert(!s.dimmed&&s.moves==0);
        for(int visit=0;visit<9&&!s.won;visit++) {
            int cursor=s.cursor;assert(!(s.fixed&(1u<<cursor))&&s.tiles[cursor]);
            for(int turns=0;turns<4&&s.tiles[cursor]!=s.solution[cursor]&&!s.won;turns++) z_action(&s,Z_OK);
            if(!s.won) z_action(&s,Z_DOWN);bounds(&s);
        }
        assert(s.won&&(s.powered&s.homes)==s.homes);
        for(int i=0;i<9;i++) if(s.fixed&(1u<<i)) assert(s.tiles[i]==fixed[i]);
        int moves=s.moves;z_action(&s,Z_UP);assert(s.moves==moves&&s.won);
        z_action(&s,Z_OK);assert(s.page==Z_PLAY&&s.level==(level+1)%Z_LEVELS);
    }
    puts("PASS six solvable levels, fixed tiles, untimed play, wrap selection, next level");
    for(int key=0;key<3;key++) {
        z_init(&s,42);in=(z_input){0};advance(&s,60000);assert(s.dimmed);
        int previous=s.expression;z_button(&s,&in,key,0);assert(s.page==Z_WAKE&&!s.dimmed&&s.expression!=previous);
        advance(&s,2999);assert(s.page==Z_WAKE);bounds(&s);
        advance(&s,1);assert(s.page==Z_HOME&&s.home_fade_ms==240);
        z_button(&s,&in,key,3);z_button(&s,&in,key,4);assert(s.page==Z_HOME&&s.selection==0);
        advance(&s,240);assert(!s.home_fade_ms);
        z_view v;z_render(&s,-1,&v);bool url=false;for(int i=0;i<v.count;i++) if(!strcmp(v.ops[i].text,"zco.eco")) url=true;assert(url);
    }
    puts("PASS all wake keys consumed, different expression, exact 3000 ms, home fade, URL");
    unsigned r=123;
    for(int n=0;n<100000;n++) {
        if(n%100==0) z_link_start(&s,n%6);
        r=r*1664525u+1013904223u;
        if(n%3) z_action(&s,(int)(r%3));else z_tick(&s,r%200);
        assert(s.page>=Z_HOME&&s.page<=Z_WAKE);bounds(&s);
        if(s.page==Z_LINK) { assert(s.powered==oracle(&s)); assert(s.won==((s.powered&s.homes)==s.homes)); }
    }
    puts("PASS 100000 transitions, independent connectivity oracle and layout bounds");
    printf("State: %zu bytes; draw list: %zu bytes\n",sizeof(s),sizeof(z_view));
}
