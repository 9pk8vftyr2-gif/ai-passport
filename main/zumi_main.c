#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "zumi/core.h"
#include "zumi/view.h"
#include "zumi/sprites.h"
#include "zumi/energy_icons.h"
#include "zumi/wake_cards.h"
#include "esp_random.h"
#include "zumi/sound.h"
#include "zumi/audio.h"
#include "zumi/talk_device.h"
#include <string.h>

typedef struct { int button, event; } input_event;
static QueueHandle_t queue;
static z_state state, before;
static z_input input;
static z_view view, previous;
static lv_obj_t *objects[Z_DRAW_MAX];
static lv_obj_t *screen;
static int battery = -1;

/* Single application owner, no page tasks/timers or dangling page callbacks. */
static void on_button(bsp_btn_t button, bsp_btn_ev_t event, void *user) {
    (void)user;
    const input_event item = {button, event};
    if (event == BSP_BTN_CLICK || event == BSP_BTN_DOUBLE) return;
    (void)xQueueSend(queue, &item, 0);
}
static void render(void) {
    z_render(&state, battery, &view);
    for (int i=0; i<Z_DRAW_MAX; i++) {
        if (i >= view.count) {
            if (objects[i]) { lv_obj_delete(objects[i]); objects[i]=NULL; }
            continue;
        }
        z_draw *d=&view.ops[i];
        bool fresh=!objects[i] || previous.ops[i].kind!=d->kind;
        if (!fresh && memcmp(d,&previous.ops[i],sizeof(*d))==0) continue;
        if (fresh) {
            if(objects[i]) lv_obj_delete(objects[i]);
            objects[i]=d->kind==Z_TEXT ? lv_label_create(screen) :
                       (d->kind==Z_SPRITE || d->kind==Z_WAKE_CARD || d->kind==Z_ENERGY_ICON) ? lv_image_create(screen) : lv_obj_create(screen);
            lv_obj_remove_style_all(objects[i]);
            lv_obj_remove_flag(objects[i],LV_OBJ_FLAG_SCROLLABLE);
        }
        lv_obj_t *o=objects[i];
        lv_obj_set_pos(o,d->x,d->y);
        /* Image opacity blends directly into the BSP draw buffer, without an
           object-opacity layer large enough to exhaust the 24 KB LVGL pool. */
        if(d->kind==Z_WAKE_CARD || d->kind==Z_ENERGY_ICON) lv_obj_set_style_image_opa(o,(lv_opa_t)d->opacity,0);
        else lv_obj_set_style_opa(o,(lv_opa_t)d->opacity,0);
        if(d->kind==Z_RECT) {
            lv_obj_set_size(o,d->w,d->h);
            lv_obj_set_style_bg_color(o,lv_color_hex(d->color),0);
            lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
            lv_obj_set_style_radius(o,d->radius,0);
        } else if(d->kind==Z_TEXT) {
            lv_obj_set_width(o,d->w);
            lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP);
            lv_obj_set_style_text_font(o,d->size==20?&lv_font_montserrat_20:
                                       d->size==10?&lv_font_montserrat_10:&lv_font_montserrat_14,0);
            lv_obj_set_style_text_color(o,lv_color_hex(d->color),0);
            lv_label_set_text(o,d->text);
        } else if(d->kind==Z_ENERGY_ICON) {
            lv_image_set_src(o,&zumi_energy_icons[d->size % 6]);
            lv_image_set_pivot(o,0,0);
            lv_image_set_scale(o,(uint16_t)(d->w*256/32));
        } else if(d->kind==Z_WAKE_CARD) {
            lv_image_set_src(o,&zumi_wake_cards[d->size % Z_WAKE_CARDS]);
        } else {
            lv_image_set_src(o,&zumi_sprites[d->size % 4]);
            lv_image_set_pivot(o,0,0);
            lv_image_set_scale(o,(uint16_t)(d->w*256/78));
        }
        /* Recreated nodes must retain draw-list order. */
        lv_obj_move_to_index(o,i);
    }
    previous=view;
}
static void talk_dispatch(void) {
    int c=z_talk_take_command(&state);
    if(c==T_F_ABORT&&(state.page!=Z_TALK||state.screen_off||state.talk.state!=T_IDLE))c=5;
    if(c)z_talk_device_command(c,state.talk.turn);
}
void app_main(void) {
    z_init(&state,esp_random());
    if(bsp_display_init()!=ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE("zumi","Display initialization failed"); return;
    }
    if(bsp_battery_init()==ESP_OK) battery=bsp_battery_soc();
    queue=xQueueCreate(32,sizeof(input_event));
    if(!queue) { ESP_LOGE("zumi","Input queue allocation failed"); return; }
    if(!bsp_lvgl_lock(1000)) { vQueueDelete(queue); queue=NULL; return; }
    screen=lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen,BSP_LCD_W,BSP_LCD_H);
    lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen,lv_color_hex(0xF6F3E9),0);
    lv_obj_set_style_bg_opa(screen,LV_OPA_COVER,0);
    lv_screen_load(screen);
    render();
    bsp_lvgl_unlock();
    bsp_display_backlight(70);
    if(bsp_button_init(on_button,NULL)!=ESP_OK) {
        ESP_LOGE("zumi","Buttons unavailable");
        if(bsp_lvgl_lock(1000)) {
            lv_obj_t *error=lv_label_create(screen);
            lv_label_set_text(error,"Buttons unavailable");
            lv_obj_set_pos(error,30,298);
            bsp_lvgl_unlock();
        }
        return;
    }
    z_audio_start();
    uint64_t last=esp_timer_get_time()/1000, battery_time=last;
    int brightness=70;
    for(;;) {
        uint64_t now=esp_timer_get_time()/1000;
        z_tick(&state,(uint32_t)(now-last)); last=now;
        talk_dispatch();z_talk_device_poll(&state);talk_dispatch();
        input_event e;
        /* Bounded draining ensures an event flood cannot starve rendering. */
        for(int i=0;i<32 && xQueueReceive(queue,&e,0)==pdTRUE;i++) {
            before=state;
            z_button(&state,&input,e.button,e.event);talk_dispatch();
            int sound=z_sound_event(&before,&state);
            if(sound || (before.page==Z_LINK && (state.page!=Z_LINK || before.level!=state.level))) z_audio_play(sound);
        }
        if(state.screen_off && brightness) {
            bsp_display_backlight(0); brightness=0;
            z_audio_play(Z_SOUND_NONE);
        }
        if(now-battery_time>=30000) { battery=bsp_battery_soc(); battery_time=now; }
        if(!state.screen_off && bsp_lvgl_lock(50)) {
            render();
            if(!brightness) {
                /* Prepare the wake card while dark, then restore the backlight. */
                lv_refr_now(NULL);
                bsp_display_backlight(70); brightness=70;
            }
            bsp_lvgl_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
