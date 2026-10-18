#include "view.h"
#include <stdio.h>
#include <string.h>
#define INK 0x173F37
#define PAPER 0xF6F3E9
#define GREEN 0xD5E9BA
#define MUTED 0x65766B
static z_draw *add(z_view *v, int kind, int x, int y, int w, int h, int color) {
    if (v->count >= Z_DRAW_MAX) return NULL;
    z_draw *d = &v->ops[v->count++];
    *d = (z_draw){.kind=kind,.x=x,.y=y,.w=w,.h=h,.color=color,.opacity=255};
    return d;
}
static void box(z_view *v, int x, int y, int w, int h, int color, int radius) {
    z_draw *d = add(v,Z_RECT,x,y,w,h,color); if (d) d->radius=radius;
}
static void text(z_view *v, int x, int y, const char *str, int color, int size) {
    z_draw *d=add(v,Z_TEXT,x,y,240-x-12,size+6,color);
    if(d) { d->size=size; snprintf(d->text,sizeof(d->text),"%s",str); }
}
static void sprite(z_view *v, int x, int y, int mood) {
    z_draw *d=add(v,Z_SPRITE,x,y,78,92,0); if(d) d->size=mood;
    text(v,165,y+84,"zco.eco",MUTED,10);
}
static void choice(z_view *v,int y,const char *label,bool selected) {
    box(v,20,y,200,32,selected?INK:0xE8EBDD,10);
    text(v,34,y+7,label,selected?PAPER:INK,14);
    if(selected) text(v,198,y+7,">",PAPER,14);
}
void z_render(const z_state *s, int battery, z_view *v) {
    v->count=0;
    box(v,0,0,240,320,PAPER,0);
    text(v,22,16,"ZUMI POCKET",INK,14);
    char tmp[64];
    if(battery<0) snprintf(tmp,sizeof(tmp),"--"); else snprintf(tmp,sizeof(tmp),"%d%%",battery);
    text(v,184,16,tmp,MUTED,14);
    if(s->page==Z_WAKE) {
        z_draw *hero=add(v,Z_SPRITE,61,83,117,138,0);
        if(hero) hero->size=s->expression;
        text(v,66,241,"Hello again.",INK,14);
        text(v,171,292,"zco.eco",MUTED,10);
    } else if(s->page==Z_HOME) {
        sprite(v,81,46,s->expression);
        text(v,55,145,"A little greener.",INK,14);
        choice(v,180,"Talk",s->selection==0);
        choice(v,218,"Play",s->selection==1);
        choice(v,256,"Garden",s->selection==2);
        text(v,36,298,"UP / DOWN    OK select",MUTED,14);
    } else if(s->page==Z_PLAY) {
        text(v,24,49,"Energy Link",INK,20);
        sprite(v,81,81,s->expression);
        text(v,25,189,"Turn wires. Light homes.",INK,14);
        snprintf(tmp,sizeof(tmp),"<   Level %d / 6   >",s->level+1);
        text(v,43,217,tmp,MUTED,14);
        choice(v,247,"Start connecting",true);
        text(v,25,289,"UP / DOWN choose level",MUTED,14);
    } else if(s->page==Z_TALK || s->page==Z_GARDEN) {
        bool talk=s->page==Z_TALK;
        text(v,24,55,talk?"Hello, friend.":"Our little garden.",INK,20);
        sprite(v,81,97,s->expression);
        text(v,28,205,talk?"Voice is not connected.":"Garden is not connected.",INK,14);
        text(v,28,231,"Your games work offline.",MUTED,14);
        choice(v,263,"Back home",true);
    } else if(s->page==Z_LINK) {
        text(v,22,46,"Energy Link",INK,20);
        snprintf(tmp,sizeof(tmp),"%d / 6    %d turns",s->level+1,s->moves);
        text(v,22,74,tmp,MUTED,14);
        /* Shared edges meet exactly, even across rounded cell backgrounds. */
        for(int i=0;i<9;i++) {
            int x=24+(i%3)*64,y=101+(i/3)*48;
            bool lit=(s->powered&(1u<<i))!=0;
            if(i==s->cursor&&!s->won) box(v,x,y,64,48,INK,9);
            box(v,x+2,y+2,60,44,lit?GREEN:0xE8EBDD,7);
            int c=lit?0x477A35:MUTED,m=s->tiles[i];
            if(m&1) box(v,x+30,y,4,26,c,0);
            if(m&2) box(v,x+30,y+22,34,4,c,0);
            if(m&4) box(v,x+30,y+22,4,26,c,0);
            if(m&8) box(v,x,y+22,34,4,c,0);
            if(i==s->source || (s->homes&(1u<<i))) {
                box(v,x+20,y+12,24,24,lit?INK:0xB8C1AE,6);
                text(v,x+26,y+17,i==s->source?"S":"H",PAPER,14);
            } else if(s->fixed&(1u<<i)) box(v,x+6,y+6,4,4,INK,0);
        }
        if(s->won) {
            text(v,27,254,"All homes are glowing!",INK,14);
            choice(v,278,s->level==5?"Play from level 1":"Next level",true);
        } else {
            text(v,27,254,"UP / DOWN select wire",MUTED,14);
            text(v,27,275,"OK turn clockwise",INK,14);
            text(v,27,299,"Hold OK: home",MUTED,10);
        }
    }
    /* Fade individual primitives; no off-screen full-frame buffer needed. */
    int opacity=255;
    if(s->page==Z_WAKE) {
        if(s->wake_ms<240) opacity=(int)(s->wake_ms*255/240);
        else if(s->wake_ms>2640) opacity=(int)((3000-s->wake_ms)*255/360);
    } else if(s->page==Z_HOME && s->home_fade_ms)
        opacity=(int)((240-s->home_fade_ms)*255/240);
    for(int i=3;i<v->count;i++) v->ops[i].opacity=opacity;
}
