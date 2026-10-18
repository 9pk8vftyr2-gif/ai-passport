/* Exhaustive orientation search, independent bit-mask connectivity oracle.
 * Reports a real minimum number of clockwise turns (selection presses excluded).
 * All orientation combinations cheaper than the known solution are searched. */
#include "core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static z_state level;
static int best, count, slots[Z_CELLS], counts[Z_CELLS];
static uint8_t values[Z_CELLS][4], costs[Z_CELLS][4], answer[Z_CELLS], current[Z_CELLS];
static unsigned long leaves;
static int connected(uint16_t n,uint16_t e,uint16_t s,uint16_t w) {
    uint16_t h=e&(w>>1)&0x7777u, v=s&(n>>4);
    uint16_t seen=(uint16_t)(1u<<level.source), old;
    do { old=seen;seen|=(seen&h)<<1;seen|=(seen>>1)&h;
         seen|=(seen&v)<<4;seen|=(seen>>4)&v;
    } while(seen!=old);
    return (seen&level.homes)==level.homes;
}
static void search(int depth,int cost,uint16_t n,uint16_t e,uint16_t s,uint16_t w) {
    if(cost>=best) return;
    if(depth==count) {
        leaves++;
        if(connected(n,e,s,w)) {best=cost;for(int i=0;i<Z_CELLS;i++)answer[i]=current[i];}
        return;
    }
    int cell=slots[depth];uint16_t bit=(uint16_t)(1u<<cell);
    for(int j=0;j<counts[depth];j++) {
        int m=values[depth][j];current[cell]=(uint8_t)m;
        search(depth+1,cost+costs[depth][j],n|((m&1)?bit:0),e|((m&2)?bit:0),
               s|((m&4)?bit:0),w|((m&8)?bit:0));
    }
}
int main(void) {
    assert(Z_COLS==4&&Z_ROWS==4);
    int previous_minimum=0;
    puts("[");
    for(int l=0;l<Z_LEVELS;l++) {
        z_init(&level,42);z_link_start(&level,l);count=0;best=1;leaves=0;
        uint16_t n=0,e=0,s=0,w=0;
        for(int i=0;i<Z_CELLS;i++) {
            int m=level.tiles[i],turns=0;
            while(m!=level.solution[i]&&turns<4){m=((m<<1)|(m>>3))&15;turns++;}
            assert(turns<4);best+=turns;
            current[i]=level.tiles[i];answer[i]=level.solution[i];m=level.tiles[i];
            if((level.fixed&(1u<<i))||!m) {
                assert(!turns);uint16_t bit=(uint16_t)(1u<<i);
                if(m&1)n|=bit;if(m&2)e|=bit;if(m&4)s|=bit;if(m&8)w|=bit;
            } else {
                slots[count]=i;counts[count]=0;
                do {int j=counts[count]++;values[count][j]=(uint8_t)m;costs[count][j]=(uint8_t)j;
                    m=((m<<1)|(m>>3))&15;} while(m!=level.tiles[i]);
                count++;
            }
        }
        search(0,0,n,e,s,w);assert(best>=5&&best>previous_minimum);previous_minimum=best;
        z_state proof=level;for(int i=0;i<Z_CELLS;i++)proof.tiles[i]=answer[i];
        z_link_evaluate(&proof);assert(proof.won);
        printf("{\"level\":%d,\"minimum_turns\":%d,\"searched_leaves\":%lu,\"initial\":[",l+1,best,leaves);
        for(int i=0;i<Z_CELLS;i++)printf("%s%d",i?",":"",level.tiles[i]);
        printf("],\"solution\":[");
        for(int i=0;i<Z_CELLS;i++)printf("%s%d",i?",":"",answer[i]);
        printf("],\"fixed\":%u}%s\n",level.fixed,l==Z_LEVELS-1?"":",");
    }
    puts("]");
}
