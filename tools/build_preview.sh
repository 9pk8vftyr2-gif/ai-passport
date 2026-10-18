#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${WASI_SDK_PATH:?Set WASI_SDK_PATH to wasi-sdk 34 (or a compatible SDK)}"
"$WASI_SDK_PATH/bin/clang" -O2 -Wall -Wextra -Werror -Imain/zumi \
    -mexec-model=reactor main/zumi/core.c main/zumi/view.c main/zumi/sound.c main/zumi/sounds.c preview/bridge.c \
    -Wl,--export=pocket_take_sound -Wl,--export=pocket_sound_frames -Wl,--export=pocket_sound_chunk \
    -Wl,--export=pocket_card -Wl,--export=pocket_cards_done -Wl,--export=pocket_card_count \
    -Wl,--export=pocket_init -Wl,--export=pocket_button \
    -Wl,--export=pocket_tick -Wl,--export=pocket_render \
    -Wl,--export=pocket_ops -Wl,--export=pocket_stride \
    -Wl,--export=pocket_page -Wl,--export=pocket_moves \
    -Wl,--export=pocket_won -Wl,--export=pocket_cursor \
    -Wl,--export=pocket_board -Wl,--export=pocket_level -Wl,--export=pocket_dimmed \
    -Wl,--export=pocket_expression -o preview/pocket.wasm
chmod 644 preview/pocket.wasm
python3 - <<'PY'
import hashlib, json
from pathlib import Path
files = ['main/zumi/core.c', 'main/zumi/core.h', 'main/zumi/view.c',
         'main/zumi/view.h', 'main/zumi/sound.h', 'main/zumi/sound.c', 'main/zumi/sounds.c', 'preview/bridge.c', 'preview/pocket.wasm']
hashes = {p: hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in files}
Path('preview/build-manifest.json').write_text(json.dumps(hashes, indent=2) + '\n')
PY
echo 'Browser module built from the firmware C sources.'
