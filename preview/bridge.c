#include "core.h"
#include "view.h"
static z_state state;
static z_input input;
static z_view view;
void pocket_init(unsigned seed) { z_init(&state,seed); input=(z_input){0}; }
void pocket_button(int button,int event) { z_button(&state,&input,button,event); }
void pocket_tick(unsigned ms) { z_tick(&state,ms); }
int pocket_render(void) { z_render(&state,-1,&view); return view.count; }
const z_draw *pocket_ops(void) { return view.ops; }
int pocket_stride(void) { return sizeof(z_draw); }
int pocket_page(void) { return state.page; }
int pocket_score(void) { return state.score; }
int pocket_lives(void) { return state.lives; }
int pocket_lane(void) { return state.lane; }
int pocket_target(void) { return state.carbon_lane; }
