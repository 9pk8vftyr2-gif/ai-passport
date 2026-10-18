#include "wake_cards.h"
/* Linked into read-only Flash. LVGL reads rows directly; no decoded frame in RAM. */
extern const uint8_t wake_pixels[] asm("_binary_wake_cards_rgb565_start");
#define CARD(i) {.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565, \
    .w=240,.h=320,.stride=480},.data_size=153600,.data=wake_pixels+(i)*153600}
const lv_image_dsc_t zumi_wake_cards[Z_WAKE_CARDS] = {
    CARD(0), CARD(1), CARD(2), CARD(3), CARD(4), CARD(5), CARD(6), CARD(7)
};
