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
int pocket_moves(void) { return state.moves; }
int pocket_cursor(void) { return state.cursor; }
int pocket_level(void) { return state.level; }
int pocket_won(void) { return state.won; }
int pocket_board(void) { int n=0; for(int i=0;i<9;i++) n=((int64_t)n*17+state.tiles[i])%1000000007; return n; }
int pocket_dimmed(void) { return state.dimmed; }
int pocket_expression(void) { return state.expression; }
