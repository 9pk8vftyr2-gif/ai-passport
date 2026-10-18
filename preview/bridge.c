#include "core.h"
#include "view.h"
#include "sound.h"
static int pending_sound;
static int16_t sound_chunk[256];
static z_state state;
static z_input input;
static z_view view;
void pocket_init(unsigned seed) { z_init(&state,seed); input=(z_input){0}; pending_sound=0; }
void pocket_button(int button,int event) {
    z_state before=state;z_button(&state,&input,button,event);
    int sound=z_sound_event(&before,&state);
    if(sound) pending_sound=sound;
    if(state.page!=Z_LINK) pending_sound=0;
}
int pocket_take_sound(void) { int sound=pending_sound;pending_sound=0;return sound; }
int pocket_sound_frames(int event) { return z_sound_frames(event); }
const int16_t *pocket_sound_chunk(int event,int frame) {
    for(int i=0;i<256;i++) sound_chunk[i]=z_sound_sample(event,frame+i);
    return sound_chunk;
}
void pocket_tick(unsigned ms) { z_tick(&state,ms); }
int pocket_render(void) { z_render(&state,-1,&view); return view.count; }
const z_draw *pocket_ops(void) { return view.ops; }
int pocket_stride(void) { return sizeof(z_draw); }
int pocket_page(void) { return state.page; }
int pocket_moves(void) { return state.moves; }
int pocket_cursor(void) { return state.cursor; }
int pocket_level(void) { return state.level; }
int pocket_won(void) { return state.won; }
int pocket_board(void) { int n=0; for(int i=0;i<9;i++) n=((int64_t)n*17+state.tiles[i])%1000000007; return n; }
int pocket_screen_off(void) { return state.screen_off; }
int pocket_expression(void) { return state.expression; }
int pocket_wake_card(void) { return state.wake_card; }
