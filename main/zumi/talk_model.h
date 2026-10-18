#pragma once
/* Review-stage model: built by host/WASM tests, intentionally not in firmware. */
#include <stdbool.h>
#include <stdint.h>
enum { T_IDLE, T_LISTENING, T_PROCESSING, T_SPEAKING, T_OFFLINE, T_ERROR };
enum { T_OK, T_CANCEL, T_CONNECTED, T_DISCONNECTED, T_ASR, T_REPLY,
       T_AUDIO, T_DONE, T_FAILURE, T_OVERFLOW, T_EVENT_COUNT };
enum { T_F_NONE, T_F_BEGIN, T_F_COMMIT, T_F_ABORT, T_F_CONNECT };
enum { T_E_NONE, T_E_TIMEOUT, T_E_NETWORK, T_E_BUSY, T_E_SERVICE, T_E_OVERFLOW };
enum { T_RECORD_MS=15000, T_PROCESS_MS=20000, T_SPEAK_MS=60000,
       T_AUDIO_GAP_MS=5000, T_CONNECT_MS=10000,
       T_ASR_CAP=1024, T_REPLY_CAP=1536, T_FRAME_BYTES=640,
       T_UPLINK_FRAMES=5, T_DOWNLINK_FRAMES=12 };
typedef struct {
    uint32_t turn, elapsed, silence;
    int state, effect, error;
    bool online, mic, speaker, connecting;
    char asr[T_ASR_CAP], reply[T_REPLY_CAP];
} t_model;
void t_init(t_model *m);
/* Network events carry the current turn; stale events cannot revive audio. */
void t_event(t_model *m, int event, uint32_t turn, const char *text);
void t_tick(t_model *m, uint32_t ms);
