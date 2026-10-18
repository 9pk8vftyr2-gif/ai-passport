#include <stdio.h>
#include <stdint.h>
void pocket_init(unsigned);
void pocket_button(int,int);
void pocket_tick(unsigned);
int pocket_page(void),pocket_score(void),pocket_lives(void),pocket_lane(void),pocket_target(void);
int main(void) {
    uint32_t random=123;
    pocket_init(42);
    for(int i=0;i<5000;i++) {
        if(i%250==0) {
            pocket_init(42+(unsigned)i);
            pocket_button(1,0); pocket_button(2,0); pocket_button(2,4);
            if(i%500) pocket_button(1,0);
            pocket_button(2,0); pocket_button(2,4);
        }
        random=random*1664525u+1013904223u;
        int key=(int)(random%3);
        pocket_button(key,0);pocket_button(key,4);pocket_tick(20);
        printf("%d,%d,%d,%d,%d\n",pocket_page(),pocket_score(),pocket_lives(),pocket_lane(),pocket_target());
    }
}
