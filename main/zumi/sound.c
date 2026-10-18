#include "sound.h"
extern const int16_t z_tone_turn[1440],z_tone_connect[1440],z_tone_win[7520];
static int homes(const z_state *s) {
    unsigned n=s->powered&s->homes;int count=0;
    while(n){count+=(int)(n&1u);n>>=1;}return count;
}
int z_sound_event(const z_state *before,const z_state *after) {
    if(before->page!=Z_LINK || after->page!=Z_LINK || after->moves!=before->moves+1) return Z_SOUND_NONE;
    if(after->won && !before->won) return Z_SOUND_WIN;
    return homes(after)>homes(before)?Z_SOUND_CONNECT:Z_SOUND_TURN;
}
int z_sound_frames(int event) { return event==Z_SOUND_WIN?17440:(event==Z_SOUND_TURN||event==Z_SOUND_CONNECT)?1440:0; }
int16_t z_sound_sample(int event,int frame) {
    if(frame<0||frame>=z_sound_frames(event)) return 0;
    if(event==Z_SOUND_TURN) return z_tone_turn[frame];
    if(event==Z_SOUND_CONNECT || frame<1440) return z_tone_connect[frame];
    /* Website victory chord starts 620 ms after the winning rotation. */
    return frame<9920?0:z_tone_win[frame-9920];
}
