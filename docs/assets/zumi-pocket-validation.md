[简体中文](zumi-pocket-validation.zh_CN.md)

# Zumi Pocket Validation

2026-10-18. 4 x 4 board, harder six-level progression and Eco Pop icon increment on `feature/zumi-pocket`, based on
`1c7a2cf`. The successful gate ran before committing, so the image version is
`1c7a2cf-dirty`; no firmware source or display pixel changes followed the build.
The full-image and ELF hashes below identify the actual deliverable.

| Field | Status | Evidence |
| --- | --- | --- |
| Build | PASS | ESP-IDF 5.5.3, complete official gate, merged layout and matching ELF archive verified |
| Host Tests | PASS | Original repository checks, all six 16-cell solutions/fourth-row links/row boundaries/full cursor coverage, sanitizers, 100,000 game transitions, all-page 60-second boundaries/activity reset/late-key guards, 256 wake cycles and all eight cards, 5,000 native/WASM comparisons, provenance and pixel-data hashes |
| Browser Demo | PASS | Chrome desktop/mobile; redesigned home, six 4-column x 4-row boards and previews, direct next level; black pixels and in-game screen-off/wake with unchanged progress and continued play, eight distinct cards offline, no adjacent repeats, three-second return, game controls, full English labels and shared sound |
| Device Tests | NOT RUN | No physical device flashed, erased, reset or opened |
| Unverified | Pending hardware | physical 48-pixel-cell icon/cursor readability, backlight shutdown and wake latency, LCD colors and clipping, fade frame time, internal heap/LVGL peak, audio, battery current and real runtime |

## Exact firmware identity

- Application: 2,006,8104 bytes of the 8,323,072-byte factory partition.
- Merged image: 2,072,432 bytes. Flash configuration: 8 MB; no PSRAM.
- Eight wake cards: 1,228,800 read-only Flash bytes, native 240 x 320 RGB565.
- Basic sprites: 43,056 Flash bytes; shared audio PCM: 20,800 bytes.
- Eco Pop icons: 12,288 Flash bytes (six 32 x 32 RGB565 lit/unlit variants).
- State: 104 bytes; two draw lists: 12,808 bytes. Original 9,600-byte BSP draw
  buffer and 24 KB LVGL pool retained. No added full-frame decoder buffer.
- Full-image SHA-256: `cb681e7452c32b843511904bff3760614e3aac0f3123296854623f2f94aadd2a`.
- Matching ELF SHA-256: `34e33e22cbaf63ab37d335075f6ea558b17488fb3c6d3f1003cdab5b21c16ddc`.
- Local bundle: `build/firmware/cb681e7452c32b843511904bff3760614e3aac0f3123296854623f2f94aadd2a/`.

The archive contains the matching merged/application images, ELF, MAP, bootloader,
partition table, flash arguments and manifest. It is ignored by Git and has not
been published. Verify with `python3 tools/archive_firmware.py verify` followed
by the archive path. The firmware uses the original BSP and unchanged partition
layout; no Xiaozhi SDK was used.

## Actual validation

`./tools/validate.sh` completed with ESP-IDF 5.5.3; log:
`build/full-gate-4x4-eco-icons.log`.

`tests/test_zumi.c` also passed with AddressSanitizer and UndefinedBehaviorSanitizer.
It checks all six solvable levels, a reciprocal-edge oracle, fixed tiles, 3,000 ms
wake timing, key suppression, all eight wake-card selections without adjacent
repeats, fade boundaries, Garden placeholder and audio event/PCM bounds.
`node tests/wasm_parity.cjs` compares 5,000 transitions including idle/wake cycles.
`tests/zumi_difficulty.c` exhaustively proves minimum clockwise rotations of
**8, 13, 16, 18, 20, 25** for the six boards, excluding cursor-selection presses.
It is included in the full gate; results: `build/zumi-difficulty.json`.
`tests/test_zumi_artifacts.py` validates Eco Pop source/PNG/firmware-pixel hashes, source/module hashes, eight distinct
original sticker hashes, approved source hashes, PNG dimensions and RGB565 slices.

`node tests/browser_zumi.cjs` passed after loading local assets and switching the
browser offline. Evidence: `build/browser-results.json`, `browser-screen-off.png`, `browser-game-resumed.png`, `browser-play-title.png`,
`browser-wake-card-1.png` through `browser-wake-card-8.png`, `browser-link.png`,
`browser-wind.png` and `browser-mobile.png`. Browser previews do not establish
physical LCD, battery or audio performance.

## Behavior and delivery boundary

Eight approved V3 promotional cards replace the earlier Tiny Actions checklist.
They reuse eight distinct original local Office Vibes sticker expressions/actions;
all preserve Zumi Pocket and zco.eco. The generated backgrounds were approved;
no paid AI service is connected to the firmware. Local package provenance does
not assert published WeChat status. Native cards are read directly from Flash;
image opacity avoids a full-frame object layer. Garden and Talk are placeholders.
Energy Link titles sit at (22,16), clear of battery information. Home now uses
three numbered entries without a character. The initial level picker replaces
the character with a preview of the selected board. All six boards and previews now have four columns and four rows. The level/turn
counter moves to (22,47); the board starts at y=72 with 48 x 44 pixel cells.
Preview cells are 48 x 34 pixels. Each solvable level uses the fourth row.
Board screenshots use 38–55 commands, below the 64-command limit, using the original Eco Pop sun, turbine and house artwork with a full English footer legend.
Additional tests cover fourth-row horizontal/vertical links, row-edge isolation,
all movable cursor positions and wake preservation across all 16 cells.
After a win, OK starts the next board directly, wrapping after level six; there
is no inter-level picker. Initial Play entry still allows level selection.
Changing levels cancels previous game audio. Four-row screenshots:
`build/browser-board-4x4-1.png` through `browser-board-4x4-6.png`; the level/turn counter retains its raised position. Other screenshots:
`build/browser-home-redesign.png`, `build/browser-level-preview-1.png` through
`browser-level-preview-6.png`.

All pages turn the backlight off (70% to 0%) after 60 seconds without input,
and application redraws stop. Any key wakes to a random card for three seconds,
then the original game, or home when woken from other pages. Game level, wires,
moves, cursor and completion state are preserved. The resumed page fades in over
240 ms and starts a new 60-second idle interval. Added tests cover six levels,
all three keys and three consecutive wakes each, exact three-second timing,
held-key guards, completed boards, continued rotation and next-level actions. Tests cover
exact boundaries, activity reset, all wake keys, late long/release guards and
repeated wakes. The wake frame is updated before restoring brightness; physical
old-frame flashes still require checking. Screen-off cancels queued sound, but
leaves initialized audio hardware active. CPU, LCD controller and button polling
continue: this is backlight-off standby, not deep sleep. The official 500 mAh
capacity alone cannot establish runtime; measure lit, screen-off and game board
currents separately. No motion detector or hourly reminder was enabled.
No ZCO website edit, Cloudflare deployment or device operation.
Fork/upstream main were both verified unchanged at
`3d685a90bdb96dd8526d84edde1e63fec25b460e`.
See the [hardware-arrival checklist](zumi-pocket.md#hardware-arrival-checklist--not-run).
