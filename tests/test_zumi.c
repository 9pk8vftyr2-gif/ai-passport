#include "core.h"
#include "view.h"
#include "sound.h"
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
        for(int i=0;i<Z_CELLS;i++) {
            if(i%Z_COLS<Z_COLS-1 && (s->tiles[i]&2) && (s->tiles[i+1]&8) && (seen&((1u<<i)|(1u<<(i+1))))) seen|=(1u<<i)|(1u<<(i+1));
            if(i<Z_CELLS-Z_COLS && (s->tiles[i]&4) && (s->tiles[i+Z_COLS]&1) && (seen&((1u<<i)|(1u<<(i+Z_COLS))))) seen|=(1u<<i)|(1u<<(i+Z_COLS));
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
        z_link_start(&s,level);assert(!s.won);uint8_t fixed[Z_CELLS];memcpy(fixed,s.tiles,sizeof(fixed));
        z_state solved=s;memcpy(solved.tiles,solved.solution,sizeof(solved.tiles));
        z_link_evaluate(&solved);assert(solved.won&&solved.powered==oracle(&solved));
        assert(solved.powered & (7u << 9)); /* Each solution uses the fourth row. */
        z_state picker=s;picker.page=Z_PLAY;z_state untouched=picker;
        bounds(&picker);assert(!memcmp(&picker,&untouched,sizeof(picker)));
        unsigned visited=0,expected=0;
        for(int i=0;i<Z_CELLS;i++) if(s.tiles[i]&&!(s.fixed&(1u<<i))) expected|=1u<<i;
        for(int i=0;i<Z_CELLS;i++){visited|=1u<<s.cursor;z_action(&s,Z_DOWN);}
        assert(visited==expected);
        z_link_start(&s,level);
        advance(&s,50000);assert(!s.screen_off&&s.moves==0);
        for(int visit=0;visit<Z_CELLS&&!s.won;visit++) {
            int cursor=s.cursor;assert(!(s.fixed&(1u<<cursor))&&s.tiles[cursor]);
            for(int turns=0;turns<4&&s.tiles[cursor]!=s.solution[cursor]&&!s.won;turns++) z_action(&s,Z_OK);
            if(!s.won) z_action(&s,Z_DOWN);bounds(&s);
        }
        assert(s.won&&(s.powered&s.homes)==s.homes);
        for(int i=0;i<Z_CELLS;i++) if(s.fixed&(1u<<i)) assert(s.tiles[i]==fixed[i]);
        int moves=s.moves;z_action(&s,Z_UP);assert(s.moves==moves&&s.won);
        z_action(&s,Z_OK);assert(s.page==Z_LINK&&s.level==(level+1)%Z_LEVELS&&s.moves==0&&!s.won);
    }
    puts("PASS six solvable levels, fixed tiles, untimed play, wrap selection, next level");
    z_init(&s,42);s.source=2;s.homes=1u<<3;s.tiles[2]=2;s.tiles[3]=8;
    z_link_evaluate(&s);assert(!s.won); /* Right edge cannot wrap into the next row. */
    z_init(&s,42);s.source=8;s.homes=1u<<11;s.tiles[8]=4;s.tiles[11]=1;
    z_link_evaluate(&s);assert(s.won&&s.powered==((1u<<8)|(1u<<11)));
    s.tiles[11]=2;z_link_evaluate(&s);assert(!s.won); /* Reciprocal bottom edge. */
    z_init(&s,42);s.source=9;s.homes=1u<<11;s.tiles[9]=2;s.tiles[10]=10;s.tiles[11]=8;
    z_link_evaluate(&s);assert(s.won&&s.powered==(7u<<9));
    puts("PASS fourth-row connectivity, no row wrap, full cursor coverage and pure previews");
    for(int key=0;key<3;key++) {
        z_init(&s,42);in=(z_input){0};advance(&s,60000);assert(s.screen_off);
        int previous=s.expression;z_button(&s,&in,key,0);assert(s.page==Z_WAKE&&!s.screen_off&&s.expression!=previous);
        advance(&s,2999);assert(s.page==Z_WAKE);bounds(&s);
        advance(&s,1);assert(s.page==Z_HOME&&s.resume_fade_ms==240);
        z_button(&s,&in,key,3);z_button(&s,&in,key,4);assert(s.page==Z_HOME&&s.selection==0);
        advance(&s,240);assert(!s.resume_fade_ms);
        z_view v;z_render(&s,-1,&v);bool url=false;for(int i=0;i<v.count;i++) if(!strcmp(v.ops[i].text,"zco.eco")) url=true;assert(url);
    }
    puts("PASS all wake keys consumed, different expression, exact 3000 ms, home fade, URL");
    for(int page=Z_HOME;page<=Z_GARDEN;page++) {
        z_init(&s,42);in=(z_input){0};s.page=page;
        if(page==Z_LINK) z_link_start(&s,1);
        advance(&s,59999);assert(!s.screen_off);
        z_button(&s,&in,Z_DOWN,0);z_button(&s,&in,Z_DOWN,4);
        advance(&s,59999);assert(!s.screen_off);
        advance(&s,1);assert(s.screen_off&&s.idle_ms==Z_IDLE_OFF_MS);
        int moves=s.moves;uint8_t tiles[Z_CELLS];memcpy(tiles,s.tiles,sizeof(tiles));
        advance(&s,120000);assert(s.screen_off&&s.idle_ms==Z_IDLE_OFF_MS);
        z_action(&s,Z_OK);assert(s.screen_off&&s.page==page);
        z_button(&s,&in,Z_OK,3);z_button(&s,&in,Z_OK,4);
        assert(s.screen_off&&s.page==page&&s.moves==moves&&!memcmp(tiles,s.tiles,sizeof(tiles)));
        z_button(&s,&in,Z_OK,0);z_button(&s,&in,Z_OK,4);
        assert(!s.screen_off&&s.page==Z_WAKE);
        advance(&s,3000);assert(s.page==(page==Z_LINK?Z_LINK:Z_HOME)&&s.idle_ms==0);
        advance(&s,59999);assert(!s.screen_off);advance(&s,1);assert(s.screen_off);
    }
    /* A release after holding OK across timeout must not enter a menu. */
    z_init(&s,42);in=(z_input){0};z_button(&s,&in,Z_OK,0);
    advance(&s,60000);z_button(&s,&in,Z_OK,4);
    assert(s.screen_off&&s.page==Z_HOME&&!in.ok_down);
    puts("PASS exact idle timeout on all pages, activity reset, black-screen input guard and repeat wake");
    for(int level=0;level<Z_LEVELS;level++) for(int key=0;key<3;key++) {
        z_init(&s,42);in=(z_input){0};z_link_start(&s,level);
        z_action(&s,Z_OK);z_action(&s,Z_DOWN);
        /* Level 0 is won here; other levels cover partial boards. */
        z_state saved=s;
        for(int cycle=0;cycle<3;cycle++) {
            advance(&s,60000);assert(s.screen_off);
            z_button(&s,&in,key,0);assert(s.page==Z_WAKE);
            advance(&s,2999);assert(s.page==Z_WAKE);
            advance(&s,1);assert(s.page==Z_LINK&&s.idle_ms==0);
            z_view v;z_render(&s,100,&v);assert(v.ops[2].opacity==0);
            /* Holding the wake key past the greeting must not rotate/exit/advance. */
            z_button(&s,&in,key,3);z_button(&s,&in,key,4);
            assert(s.page==Z_LINK&&s.level==saved.level&&s.moves==saved.moves);
            assert(s.cursor==saved.cursor&&s.won==saved.won&&s.powered==saved.powered);
            assert(s.homes==saved.homes&&s.fixed==saved.fixed&&s.source==saved.source&&s.wind==saved.wind);
            assert(!memcmp(s.tiles,saved.tiles,sizeof(s.tiles))&&!memcmp(s.solution,saved.solution,sizeof(s.solution)));
            assert(z_sound_event(&saved,&s)==Z_SOUND_NONE);
            advance(&s,240);z_render(&s,100,&v);assert(v.ops[2].opacity==255);
        }
        z_button(&s,&in,Z_OK,0);z_button(&s,&in,Z_OK,4);
        if(saved.won) assert(s.page==Z_LINK&&s.level==(level+1)%Z_LEVELS&&s.moves==0&&!s.won);
        else assert(s.page==Z_LINK&&s.moves==saved.moves+1);
        z_button(&s,&in,Z_OK,0);z_button(&s,&in,Z_OK,3);z_button(&s,&in,Z_OK,4);
        assert(s.page==Z_HOME);
    }
    puts("PASS exact game resume across all levels/keys, completed boards, repeated wakes, fades and continued play");
    z_link_start(&s,0);z_state before=s;
    z_action(&s,Z_DOWN);assert(z_sound_event(&before,&s)==Z_SOUND_NONE);
    before=s;z_action(&s,Z_OK);assert(z_sound_event(&before,&s)==Z_SOUND_WIN);
    before=s;z_action(&s,Z_OK);assert(z_sound_event(&before,&s)==Z_SOUND_NONE);
    z_link_start(&s,1);before=s;z_action(&s,Z_OK);assert(z_sound_event(&before,&s)==Z_SOUND_TURN);
    before=s;s.moves++;s.powered|=s.homes;assert(z_sound_event(&before,&s)==Z_SOUND_CONNECT);
    for(int event=1;event<=3;event++) {
        int peak=0,nonzero=0,frames=z_sound_frames(event);
        assert(frames==(event==3?17440:1440));
        for(int i=0;i<frames;i++) {int sample=z_sound_sample(event,i);if(sample<0)sample=-sample;if(sample>peak)peak=sample;nonzero+=sample>0;}
        assert(peak>1000&&peak<4500&&nonzero>1000);
        assert(z_sound_sample(event,-1)==0&&z_sound_sample(event,frames)==0);
    }
    for(int i=1440;i<9920;i++) assert(z_sound_sample(Z_SOUND_WIN,i)==0);
    puts("PASS sound event mapping, PCM bounds, gain, and 620 ms victory delay");
    z_init(&s,123);in=(z_input){0};unsigned seen=0;
    for(int i=0;i<256;i++) {
        int old=s.wake_card;
        advance(&s,60000);z_button(&s,&in,i%3,0);
        assert(s.page==Z_WAKE&&s.wake_card>=0&&s.wake_card<Z_WAKE_CARDS&&s.wake_card!=old);
        seen|=1u<<s.wake_card;
        advance(&s,240);z_view v;z_render(&s,100,&v);
        assert(v.ops[1].kind==Z_WAKE_CARD&&v.ops[1].size==s.wake_card&&v.ops[1].opacity==255);
        advance(&s,2401);z_render(&s,100,&v);assert(v.ops[1].opacity<255);
        advance(&s,359);assert(s.page==Z_HOME);z_button(&s,&in,i%3,4);assert(s.selection==0);
    }
    assert(seen==255);
    z_init(&s,123);z_action(&s,Z_DOWN);z_action(&s,Z_DOWN);z_action(&s,Z_OK);
    assert(s.page==Z_GARDEN&&z_garden_sync(&s)==Z_SERVICE_UNAVAILABLE);bounds(&s);
    z_action(&s,Z_OK);assert(s.page==Z_HOME);
    puts("PASS eight distinct wake cards, no adjacent repeat, fade boundaries, consumed keys and Garden placeholder");
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
