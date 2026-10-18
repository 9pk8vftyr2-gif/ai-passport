#include "core.h"
/* Pocket-authored boards; Energy Link direction convention N/E/S/W = 1/2/4/8.
   The source and homes are fixed. Every home must be reached via reciprocal
   connectors; unused wires are allowed, exactly as in the website game. */
static const struct { uint8_t initial[9], solved[9]; uint16_t homes, fixed; int source; } levels[Z_LEVELS] = {
{ {0,0,0,2,5,8,0,0,0}, {0,0,0,2,10,8,0,0,0}, 32, 40, 3 },
{ {0,9,8,3,3,0,1,0,0}, {0,6,8,6,9,0,1,0,0}, 4, 68, 6 },
{ {2,7,8,0,10,0,0,1,0}, {2,14,8,0,5,0,0,1,0}, 5, 133, 7 },
{ {0,0,4,12,5,14,1,0,1}, {0,0,4,6,10,13,1,0,1}, 260, 324, 6 },
{ {4,12,8,9,7,0,0,9,8}, {4,6,8,3,13,0,0,3,8}, 260, 261, 0 },
{ {2,9,4,3,11,7,1,0,1}, {2,12,4,6,11,13,1,0,1}, 324, 341, 0 }
};
static uint32_t random_next(z_state *s) {
    uint32_t x=s->rng; x^=x<<13; x^=x>>17; x^=x<<5; return s->rng=x;
}
void z_init(z_state *s,uint32_t seed) { *s=(z_state){.rng=seed?seed:1}; }
void z_link_evaluate(z_state *s) {
    s->powered=(uint16_t)(1u<<s->source);
    const int dr[]={-1,0,1,0}, dc[]={0,1,0,-1};
    for(int pass=0;pass<9;pass++) for(int i=0;i<9;i++) if(s->powered&(1u<<i)) {
        for(int d=0;d<4;d++) {
            int r=i/3+dr[d],c=i%3+dc[d];
            if(r<0||r>2||c<0||c>2) continue;
            int next=r*3+c;
            if((s->tiles[i]&(1u<<d)) && (s->tiles[next]&(1u<<((d+2)%4))))
                s->powered|=(uint16_t)(1u<<next);
        }
    }
    s->won=s->homes && (s->powered&s->homes)==s->homes;
}
static bool movable(const z_state *s,int i) { return s->tiles[i] && !(s->fixed&(1u<<i)); }
void z_link_start(z_state *s,int level) {
    s->level=level>=0&&level<Z_LEVELS?level:0;
    s->page=Z_LINK; s->moves=0; s->cursor=0; s->idle_ms=0; s->dimmed=0;
    s->homes=levels[s->level].homes; s->fixed=levels[s->level].fixed;
    s->source=levels[s->level].source;
    s->wind=s->level==2 || s->level==4;
    for(int i=0;i<9;i++) { s->tiles[i]=levels[s->level].initial[i]; s->solution[i]=levels[s->level].solved[i]; }
    for(int i=8;i>=0;i--) if(movable(s,i)) s->cursor=i;
    z_link_evaluate(s);
}
void z_action(z_state *s,int action) {
    if(action<Z_UP||action>Z_BACK||s->page==Z_WAKE) return;
    s->idle_ms=0; s->dimmed=0;
    if(action==Z_BACK) { s->page=Z_HOME; s->selection=0; return; }
    if(s->page==Z_HOME) {
        if(action==Z_UP&&s->selection>0) s->selection--;
        if(action==Z_DOWN&&s->selection<2) s->selection++;
        if(action==Z_OK) {
            const int pages[]={Z_TALK,Z_PLAY,Z_GARDEN};
            s->page=pages[s->selection]; s->selection=0;
            if(s->page==Z_TALK) (void)z_talk_request();
            if(s->page==Z_GARDEN) (void)z_garden_sync(s);
        }
    } else if(s->page==Z_PLAY) {
        if(action==Z_UP) s->level=(s->level+Z_LEVELS-1)%Z_LEVELS;
        if(action==Z_DOWN) s->level=(s->level+1)%Z_LEVELS;
        if(action==Z_OK) z_link_start(s,s->level);
    } else if(s->page==Z_LINK) {
        if(s->won) {
            if(action==Z_OK) { s->level=(s->level+1)%Z_LEVELS; s->page=Z_PLAY; }
            return;
        }
        if(action==Z_UP||action==Z_DOWN) {
            for(int n=0;n<9;n++) { s->cursor=(s->cursor+(action==Z_UP?8:1))%9; if(movable(s,s->cursor)) break; }
        }
        if(action==Z_OK) {
            uint8_t m=s->tiles[s->cursor];
            s->tiles[s->cursor]=(uint8_t)(((m<<1)|(m>>3))&15);
            s->moves++; z_link_evaluate(s);
        }
    } else if(action==Z_OK) { s->page=Z_HOME; s->selection=0; }
}
void z_tick(z_state *s,uint32_t ms) {
    if(ms>100) ms=100;
    if(s->page==Z_WAKE) {
        s->wake_ms+=ms;
        if(s->wake_ms>=3000) { s->page=Z_HOME; s->selection=0; s->home_fade_ms=240; s->idle_ms=0; }
        return;
    }
    s->home_fade_ms=s->home_fade_ms>ms?s->home_fade_ms-ms:0;
    if(s->page==Z_LINK) return; /* Untimed puzzles stay lit while thinking. */
    if(s->idle_ms<60000) s->idle_ms+=ms;
    if(s->idle_ms>=60000) { s->idle_ms=60000; s->dimmed=1; }
}
void z_button(z_state *s, z_input *input, int button, int event) {
    if (button < 0 || button > 2) return;
    if (input->wake_held) {
        if (event == 4 && button == input->wake_button) input->wake_held = false;
        return;
    }
    if (s->dimmed && event == 0) {
        s->expression = (s->expression + 1 + (int)(random_next(s) % 2)) % 3;
        s->dimmed = 0; s->idle_ms = 0; s->wake_ms = 0;
        s->page = Z_WAKE;
        *input = (z_input){.wake_held = true, .wake_button = button};
        return;
    }
    /* All keys during the greeting are consumed, including a held OK whose
       release happens after the automatic return home. */
    if (s->page == Z_WAKE) {
        if (event == 0) *input = (z_input){.wake_held=true,.wake_button=button};
        return;
    }
    if (event == 0 || event == 3 || event == 4) s->idle_ms = 0;
    if (button != Z_OK) {
        if (event == 0) z_action(s, button);
        return;
    }
    if (event == 0) { input->ok_down = true; input->ok_long = false; }
    if (event == 3 && input->ok_down && !input->ok_long) {
        input->ok_long = true; z_action(s, Z_BACK);
    }
    if (event == 4) {
        if (input->ok_down && !input->ok_long) z_action(s, Z_OK);
        input->ok_down = input->ok_long = false;
    }
}
int z_talk_request(void) { return Z_SERVICE_UNAVAILABLE; }
int z_garden_sync(const z_state *s) { (void)s; return Z_SERVICE_UNAVAILABLE; }
