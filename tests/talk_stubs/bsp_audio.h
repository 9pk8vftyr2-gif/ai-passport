#pragma once
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
esp_err_t bsp_audio_init(void);
esp_err_t bsp_audio_set_format(int rate,int bits,int channels);
void bsp_audio_set_volume(int volume);
esp_err_t bsp_audio_read(void *data,size_t size);
esp_err_t bsp_audio_write(const void *data,size_t size);
esp_err_t bsp_audio_sleep(void);
