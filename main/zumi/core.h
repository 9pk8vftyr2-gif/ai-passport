#pragma once
#include <stdint.h>
#include <stdbool.h>

enum { Z_HOME, Z_PLAY, Z_CARBON, Z_JOURNEY, Z_TALK, Z_GARDEN, Z_RESULT };
enum { Z_UP, Z_DOWN, Z_OK, Z_BACK };
enum { Z_HAPPY, Z_FOCUS, Z_CHEER };
enum { Z_SERVICE_UNAVAILABLE = -1 };
typedef struct { int x, lane, kind, active; } z_item;
typedef struct {
    int page, selection, game, score, lives, lane, mood, won;
    uint32_t rng, elapsed, spawn, step, jump, cooldown, carbon_age;
    int carbon_lane, leaf_lane;
    z_item items[6];
} z_state;
typedef struct { bool ok_down, ok_long; } z_input;
void z_init(z_state *s, uint32_t seed);
void z_action(z_state *s, int action);
void z_tick(z_state *s, uint32_t ms);
/* BSP event values: PRESS=0, CLICK=1, DOUBLE=2, LONG=3, RELEASE=4. */
void z_button(z_state *s, z_input *input, int button, int event);
int z_talk_request(void);
int z_garden_sync(const z_state *s);
