#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "talk_model.h"
typedef struct { uint32_t turn; uint16_t bytes; uint8_t pcm[T_FRAME_BYTES]; } z_pcm_frame;
void z_audio_start(void);
/* Latest event wins; NONE cancels pending/in-flight sound at the next chunk. */
void z_audio_play(int event);
bool z_audio_talk_prepare(void);
void z_audio_talk_mode(int mode,uint32_t turn); /* 0 cancel, 1 capture, 2 play, 3 finish capture */
bool z_audio_capture_pop(z_pcm_frame *out);
bool z_audio_output_push(const z_pcm_frame *frame);
bool z_audio_drained(void);
/* Worker has returned from the last BSP call and applied the stop revision. */
bool z_audio_stopped(void);
bool z_audio_talk_failed(void);
unsigned z_audio_pending(void);

/* Codec-write envelope; physical DMA/speaker synchronization needs board testing. */
unsigned z_audio_output_level(uint32_t turn);
