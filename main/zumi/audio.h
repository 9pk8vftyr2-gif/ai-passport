#pragma once
void z_audio_start(void);
/* Latest event wins; NONE cancels pending/in-flight sound at the next chunk. */
void z_audio_play(int event);
