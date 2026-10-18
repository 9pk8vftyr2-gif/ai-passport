#pragma once
#include "core.h"
enum { Z_SOUND_NONE, Z_SOUND_TURN, Z_SOUND_CONNECT, Z_SOUND_WIN, Z_SOUND_RATE=16000 };
int z_sound_event(const z_state *before,const z_state *after);
int z_sound_frames(int event);
int16_t z_sound_sample(int event,int frame);
