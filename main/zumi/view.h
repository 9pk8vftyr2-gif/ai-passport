#pragma once
#include "core.h"
#define Z_DRAW_MAX 64
enum { Z_RECT, Z_TEXT, Z_SPRITE, Z_WAKE_CARD };
typedef struct {
    int kind, x, y, w, h, color, radius, size, opacity;
    char text[64];
} z_draw;
typedef struct { int count; z_draw ops[Z_DRAW_MAX]; } z_view;
void z_render(const z_state *s, int battery, z_view *v);
