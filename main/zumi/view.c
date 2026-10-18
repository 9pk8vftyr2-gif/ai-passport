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
/* The level picker and playable board share geometry and full energy labels. */
static void board(z_view *v,const z_state *s,bool preview) {
    int cw=preview?60:64, ch=preview?34:44;
    for(int i=0;i<Z_CELLS;i++) {
        int x=(preview?30:24)+(i%Z_COLS)*cw, y=(preview?96:72)+(i/Z_COLS)*ch;
        bool lit=(s->powered&(1u<<i))!=0;
        if(!preview && i==s->cursor&&!s->won) box(v,x,y,cw,ch,INK,9);
        box(v,x+2,y+2,cw-4,ch-4,lit?GREEN:0xE8EBDD,7);
        int c=lit?0x477A35:MUTED,m=s->tiles[i],cx=cw/2-2,cy=ch/2-2;
        if(m&1) box(v,x+cx,y,4,cy+4,c,0);
        if(m&2) box(v,x+cx,y+cy,cw-cx,4,c,0);
        if(m&4) box(v,x+cx,y+cy,4,ch-cy,c,0);
        if(m&8) box(v,x,y+cy,cx+4,4,c,0);
        if(i==s->source || (s->homes&(1u<<i))) {
            box(v,x+5,y+(preview?3:5),cw-10,preview?28:34,lit?INK:MUTED,6);
            if(i==s->source) {
                text(v,x+(preview?15:(s->wind?14:12)),y+(preview?4:7),s->wind?"Wind":"Solar",PAPER,preview?10:14);
                text(v,x+(preview?15:17),y+(preview?18:24),"power",PAPER,10);
            } else text(v,x+(preview?14:11),y+(preview?12:15),"House",PAPER,preview?10:14);
        } else if(s->fixed&(1u<<i)) box(v,x+6,y+6,4,4,INK,0);
    }
}
void z_render(const z_state *s, int battery, z_view *v) {
    v->count=0;
    box(v,0,0,240,320,PAPER,0);
    char tmp[64];
    if(battery<0) snprintf(tmp,sizeof(tmp),"--"); else snprintf(tmp,sizeof(tmp),"%d%%",battery);
    if(s->page!=Z_WAKE) text(v,184,16,tmp,MUTED,14);
    int content_start=v->count;
    if(s->page==Z_WAKE) {
        z_draw *card=add(v,Z_WAKE_CARD,0,0,240,320,0);
        if(card) card->size=s->wake_card>=0?s->wake_card:0;
    } else if(s->page==Z_HOME) {
        text(v,24,20,"zco.eco",MUTED,10);
        text(v,24,55,"A little greener.",INK,20);
        text(v,24,84,"One small moment at a time.",MUTED,10);
        const char *labels[]={"Talk","Play","Garden"};
        const char *hints[]={"Say hello","Connect clean energy","A place to grow"};
        for(int i=0;i<3;i++) {
            int y=112+i*58; bool selected=s->selection==i;
            box(v,20,y,200,50,selected?INK:0xE8EBDD,12);
            snprintf(tmp,sizeof(tmp),"0%d",i+1);
            text(v,32,y+18,tmp,selected?GREEN:MUTED,14);
            text(v,66,y+6,labels[i],selected?PAPER:INK,20);
            text(v,66,y+31,hints[i],selected?GREEN:MUTED,10);
            if(selected) text(v,200,y+15,">",PAPER,14);
        }
        text(v,40,298,"UP / DOWN    OK select",MUTED,10);
    } else if(s->page==Z_PLAY) {
        text(v,22,16,"Energy Link",INK,20);
        snprintf(tmp,sizeof(tmp),"Level %d / 6",s->level+1);
        text(v,30,49,tmp,INK,20);
        text(v,30,78,"UP / DOWN choose level",MUTED,10);
        z_state preview=*s;z_link_start(&preview,s->level);
        board(v,&preview,true);
        text(v,25,238,"Turn wires. Light homes.",INK,14);
        choice(v,263,"Start connecting",true);
        text(v,45,302,"OK start   Hold OK home",MUTED,10);
    } else if(s->page==Z_GARDEN) {
        text(v,24,49,"Garden",INK,20);
        sprite(v,81,97,s->expression);
        text(v,28,205,"Garden is not connected.",INK,14);
        text(v,28,231,"Your games work offline.",MUTED,14);
        choice(v,263,"Back home",true);
    } else if(s->page==Z_TALK) {
        text(v,24,55,"Hello, friend.",INK,20);
        sprite(v,81,97,s->expression);
        text(v,28,205,"Voice is not connected.",INK,14);
        text(v,28,231,"Your games work offline.",MUTED,14);
        choice(v,263,"Back home",true);
    } else if(s->page==Z_LINK) {
        text(v,22,16,"Energy Link",INK,20);
        snprintf(tmp,sizeof(tmp),"%d / 6    %d turns",s->level+1,s->moves);
        text(v,22,47,tmp,MUTED,14);
        board(v,s,false);
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
    } else if((s->page==Z_HOME || s->page==Z_LINK) && s->resume_fade_ms)
        opacity=(int)((240-s->resume_fade_ms)*255/240);
    for(int i=content_start;i<v->count;i++) v->ops[i].opacity=opacity;
    if(s->page==Z_WAKE) text(v,184,16,tmp,MUTED,14);
}
