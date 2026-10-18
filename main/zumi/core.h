#pragma once
#include <stdint.h>
#include <stdbool.h>
enum { Z_HOME, Z_PLAY, Z_LINK, Z_TALK, Z_GARDEN, Z_WAKE };
enum { Z_UP, Z_DOWN, Z_OK, Z_BACK };
enum { Z_HAPPY, Z_FOCUS, Z_CHEER };
enum { Z_IDLE_OFF_MS = 60000 };
enum { Z_SERVICE_UNAVAILABLE = -1, Z_LEVELS = 6, Z_WAKE_CARDS = 8 };
typedef struct {
    int page, selection, level, moves, cursor, won;
    uint32_t rng, idle_ms, wake_ms, resume_fade_ms;
    int expression, screen_off;
    uint8_t tiles[9], solution[9];
    uint16_t powered, homes, fixed;
    int source, wind;
    int wake_card, wake_return_page;
} z_state;
typedef struct { bool ok_down, ok_long, wake_held; int wake_button; } z_input;
void z_init(z_state *s, uint32_t seed);
void z_action(z_state *s, int action);
void z_tick(z_state *s, uint32_t ms);
void z_button(z_state *s, z_input *input, int button, int event);
void z_link_start(z_state *s, int level);
void z_link_evaluate(z_state *s);
int z_talk_request(void);
int z_garden_sync(const z_state *s);
