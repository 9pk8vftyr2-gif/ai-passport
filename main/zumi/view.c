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
    *d = (z_draw){.kind=kind,.x=x,.y=y,.w=w,.h=h,.color=color};
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
    if(s->page==Z_HOME) {
        sprite(v,81,46,s->selection==1?Z_CHEER:s->selection==2?Z_FOCUS:Z_HAPPY);
        text(v,55,145,"A little greener.",INK,14);
        choice(v,180,"Talk",s->selection==0);
        choice(v,218,"Play",s->selection==1);
        choice(v,256,"Garden",s->selection==2);
        text(v,36,298,"UP / DOWN    OK select",MUTED,14);
    } else if(s->page==Z_PLAY) {
        text(v,24,53,"Small play.",INK,20);
        text(v,24,79,"A lighter footprint.",MUTED,14);
        sprite(v,81,104,Z_CHEER);
        choice(v,212,"Carbon Pop",s->selection==0);
        choice(v,252,"Pocket Journey",s->selection==1);
        text(v,37,298,"Hold OK to go home",MUTED,14);
    } else if(s->page==Z_TALK || s->page==Z_GARDEN) {
        bool talk=s->page==Z_TALK;
        text(v,24,55,talk?"Hello, friend.":"Our little garden.",INK,20);
        sprite(v,81,97,Z_HAPPY);
        text(v,28,205,talk?"Voice is not connected.":"Garden is not connected.",INK,14);
        text(v,28,231,"Your games work offline.",MUTED,14);
        choice(v,263,"Back home",true);
    } else if(s->page==Z_RESULT) {
        text(v,28,53,s->won?"Lovely journey!":"Let's try again.",INK,20);
        sprite(v,81,93,s->mood);
        snprintf(tmp,sizeof(tmp),"%d play points",s->score);
        text(v,56,204,tmp,INK,14);
        choice(v,253,"Play again",true);
        text(v,37,298,"Hold OK to go home",MUTED,14);
    } else {
        bool carbon=s->page==Z_CARBON;
        text(v,22,47,carbon?"Carbon Pop":"Pocket Journey",INK,20);
        snprintf(tmp,sizeof(tmp),"%d pts   %ds   %d hearts",s->score,(int)((30000-s->elapsed+999)/1000),s->lives);
        text(v,22,77,tmp,MUTED,14);
        if(carbon) {
            text(v,25,105,"Pop CO2. Protect leaves.",INK,14);
            for(int i=0;i<3;i++) {
                int y=135+i*43;
                box(v,25,y,190,36,i==s->lane?INK:0xE8EBDD,18);
                text(v,64,y+10,i==s->carbon_lane?"CO2  +10":i==s->leaf_lane?"LEAF  keep":"SUN  keep",i==s->lane?PAPER:INK,14);
            }
            box(v,25,271,190,5,0xE1E4D6,2);
            box(v,25,271,(int)(190*(1800-s->carbon_age)/1800),5,INK,2);
            text(v,30,284,"UP / DOWN aim  OK pop",MUTED,14);
        } else {
            text(v,24,103,"Collect +. Jump rocks.",INK,14);
            for(int i=0;i<3;i++) box(v,22,138+i*40,196,32,i==s->lane?GREEN:0xE8EBDD,8);
            z_draw *hero=add(v,Z_SPRITE,29,138+s->lane*40-(s->jump?8:0),26,31,0);
            if(hero) hero->size=s->jump?Z_CHEER:s->mood;
            for(int i=0;i<6;i++) if(s->items[i].active) {
                const z_item *it=&s->items[i];
                box(v,it->x,146+it->lane*40,16,18,it->kind?0x6F984B:0x8C8071,4);
                if(it->kind) text(v,it->x+3,147+it->lane*40,"+",PAPER,14);
            }
            text(v,25,269,s->cooldown?"Jump recharging...":"OK jump  UP / DOWN move",MUTED,14);
        }
        text(v,37,300,"Hold OK to go home",MUTED,14);
    }
}
