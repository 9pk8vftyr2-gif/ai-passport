#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${WASI_SDK_PATH:?Set WASI_SDK_PATH to wasi-sdk}"
"$WASI_SDK_PATH/bin/clang" -O2 -Wall -Wextra -Werror -Imain/zumi \
    -mexec-model=reactor main/zumi/talk_model.c preview/talk/bridge.c \
    -Wl,--export=talk_init -Wl,--export=talk_event -Wl,--export=talk_tick \
    -Wl,--export=talk_state -Wl,--export=talk_effect -Wl,--export=talk_error \
    -Wl,--export=talk_turn -Wl,--export=talk_elapsed -Wl,--export=talk_mic \
    -Wl,--export=talk_speaker -Wl,--export=talk_connecting -Wl,--export=talk_size \
    -Wl,--export=talk_buffer -Wl,--export=talk_asr -Wl,--export=talk_reply -Wl,--export=talk_text \
    -o preview/talk/talk.wasm
chmod 644 preview/talk/talk.wasm
python3 - <<'PY'
import hashlib,json
from pathlib import Path
files=['main/zumi/talk_model.c','main/zumi/talk_model.h','preview/talk/bridge.c','preview/talk/talk.wasm']
Path('preview/talk/manifest.json').write_text(json.dumps({p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in files},indent=2)+'\n')
PY
