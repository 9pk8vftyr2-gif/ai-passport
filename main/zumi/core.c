#include "core.h"

static uint32_t random_next(z_state *s) {
    uint32_t x = s->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return s->rng = x;
}
static void targets(z_state *s) {
    s->carbon_lane = (int)(random_next(s) % 3);
    s->leaf_lane = (s->carbon_lane + 1 + (int)(random_next(s) % 2)) % 3;
    s->carbon_age = 0;
}
void z_init(z_state *s, uint32_t seed) {
    *s = (z_state){.rng = seed ? seed : 1, .lives = 3};
}
static void start(z_state *s, int game) {
    uint32_t seed = s->rng;
    z_init(s, seed);
    s->page = s->game = game;
    s->lane = 1;
    s->mood = Z_FOCUS;
    targets(s);
}
static void finish(z_state *s) {
    s->won = s->lives > 0;
    s->page = Z_RESULT;
    s->mood = s->won ? Z_CHEER : Z_HAPPY;
}
void z_action(z_state *s, int action) {
    if (action < Z_UP || action > Z_BACK) return;
    if (action == Z_BACK) {
        s->page = Z_HOME; s->selection = 0; s->mood = Z_HAPPY;
        return;
    }
    if (s->page == Z_HOME || s->page == Z_PLAY) {
        int count = s->page == Z_HOME ? 3 : 2;
        if (action == Z_UP && s->selection > 0) s->selection--;
        if (action == Z_DOWN && s->selection < count - 1) s->selection++;
        if (action == Z_OK) {
            if (s->page == Z_PLAY) start(s, s->selection == 0 ? Z_CARBON : Z_JOURNEY);
            else {
                int pages[] = {Z_TALK, Z_PLAY, Z_GARDEN};
                s->page = pages[s->selection]; s->selection = 0;
                if (s->page == Z_TALK) (void)z_talk_request();
                if (s->page == Z_GARDEN) (void)z_garden_sync(s);
            }
        }
    } else if (s->page == Z_CARBON || s->page == Z_JOURNEY) {
        if (action == Z_UP && s->lane > 0) s->lane--;
        if (action == Z_DOWN && s->lane < 2) s->lane++;
        if (action == Z_OK && s->page == Z_CARBON) {
            if (s->lane == s->carbon_lane) { s->score += 10; s->mood = Z_CHEER; }
            else { s->lives--; s->mood = Z_FOCUS; }
            targets(s);
        } else if (action == Z_OK && !s->cooldown) {
            s->jump = 600; s->cooldown = 1000;
        }
        if (s->lives <= 0) finish(s);
    } else if (s->page == Z_RESULT && action == Z_OK) start(s, s->game);
    else if ((s->page == Z_TALK || s->page == Z_GARDEN) && action == Z_OK) {
        s->page = Z_HOME; s->selection = 0;
    }
}
static uint32_t subtract(uint32_t value, uint32_t dt) { return value > dt ? value - dt : 0; }
static void step(z_state *s) {
    s->elapsed += 20;
    s->jump = subtract(s->jump, 20);
    s->cooldown = subtract(s->cooldown, 20);
    if (s->page == Z_CARBON) {
        s->carbon_age += 20;
        if (s->carbon_age >= 1800) { s->lives--; targets(s); }
    } else {
        s->spawn += 20;
        if (s->spawn >= 1200) {
            s->spawn = 0;
            for (int i = 0; i < 6; i++) if (!s->items[i].active) {
                s->items[i] = (z_item){210, (int)(random_next(s) % 3), (int)(random_next(s) % 2), 1};
                break;
            }
        }
        for (int i = 0; i < 6; i++) {
            z_item *item = &s->items[i];
            if (!item->active) continue;
            item->x -= 2;
            if (item->x <= 40) {
                if (item->lane == s->lane) {
                    if (item->kind == 1) { s->score += 5; s->mood = Z_CHEER; }
                    else if (!s->jump) { s->lives--; s->mood = Z_FOCUS; }
                }
                item->active = 0;
            }
        }
    }
    if (s->lives <= 0 || s->elapsed >= 30000) finish(s);
}
void z_tick(z_state *s, uint32_t ms) {
    if (s->page != Z_CARBON && s->page != Z_JOURNEY) return;
    /* Bound work after a suspended tab/task; games pause rather than catch up. */
    if (ms > 100) ms = 100;
    s->step += ms;
    while (s->step >= 20 && (s->page == Z_CARBON || s->page == Z_JOURNEY)) {
        s->step -= 20; step(s);
    }
}
void z_button(z_state *s, z_input *input, int button, int event) {
    if (button < 0 || button > 2) return;
    if (button != Z_OK) {
        if (event == 0) z_action(s, button);
        return;
    }
    if (event == 0) { input->ok_down = true; input->ok_long = false; }
    if (event == 3 && input->ok_down && !input->ok_long) {
        input->ok_long = true; z_action(s, Z_BACK);
    }
    if (event == 4) {
        if (input->ok_down && !input->ok_long) z_action(s, Z_OK);
        input->ok_down = input->ok_long = false;
    }
}
int z_talk_request(void) { return Z_SERVICE_UNAVAILABLE; }
int z_garden_sync(const z_state *s) { (void)s; return Z_SERVICE_UNAVAILABLE; }
