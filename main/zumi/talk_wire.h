#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "talk_model.h"
#define T_WIRE_HEADER 14
#define T_WIRE_MAX (T_WIRE_HEADER+T_FRAME_BYTES)
size_t t_pack(uint8_t *out, int type, uint32_t turn, uint32_t seq, const void *pcm, size_t bytes);
bool t_unpack(const uint8_t *in,size_t bytes,int type,uint32_t turn,uint32_t seq,const uint8_t **pcm,size_t *length);
bool t_json_bounded(const char *text,size_t bytes);
