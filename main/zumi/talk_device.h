#pragma once
#include "core.h"
/* Commands are nonblocking. One worker owns Wi-Fi and WebSocket lifetimes. */
void z_talk_device_command(int command,uint32_t turn);
void z_talk_device_poll(z_state *state);
