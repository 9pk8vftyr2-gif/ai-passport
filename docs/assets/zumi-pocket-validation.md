[简体中文](zumi-pocket-validation.zh_CN.md)

# Zumi Pocket Validation

2026-10-18. Home/initial-picker redesign and direct-next-level increment on `feature/zumi-pocket`, based on
`20babf7`. The successful gate ran before committing, so the image version is
`20babf7-dirty`; no firmware source or display pixel changes followed the build.
The full-image and ELF hashes below identify the actual deliverable.

| Field | Status | Evidence |
| --- | --- | --- |
| Build | PASS | ESP-IDF 5.5.3, complete official gate, merged layout and matching ELF archive verified |
| Host Tests | PASS | Original repository checks, sanitizers, 100,000 game transitions, all-page 60-second boundaries/activity reset/late-key guards, 256 wake cycles and all eight cards, 5,000 native/WASM comparisons, provenance and pixel-data hashes |
| Browser Demo | PASS | Chrome desktop/mobile; redesigned home, six character-free board previews, direct next level; black pixels and in-game screen-off/wake with unchanged progress and continued play, eight distinct cards offline, no adjacent repeats, three-second return, game controls, full English labels and shared sound |
| Device Tests | NOT RUN | No physical device flashed, erased, reset or opened |
| Unverified | Pending hardware | physical backlight shutdown and wake latency, LCD colors and clipping, fade frame time, internal heap/LVGL peak, audio, battery current and real runtime |

## Exact firmware identity

- Application: 1,994,432 bytes of the 8,323,072-byte factory partition.
- Merged image: 2,059,968 bytes. Flash configuration: 8 MB; no PSRAM.
- Eight wake cards: 1,228,800 read-only Flash bytes, native 240 x 320 RGB565.
- Basic sprites: 43,056 Flash bytes; shared audio PCM: 20,800 bytes.
- State: 88 bytes; two draw lists: 12,808 bytes. Original 9,600-byte BSP draw
  buffer and 24 KB LVGL pool retained. No added full-frame decoder buffer.
- Full-image SHA-256: `c6cbbcdd92ba20af2a8e161aa502c4917be78d5fe982dea9e33ce04ec227b662`.
- Matching ELF SHA-256: `ec34af33b5fcd69509d65a958a3306760cc11c26c389a3afc5a423515987ce45`.
- Local bundle: `build/firmware/c6cbbcdd92ba20af2a8e161aa502c4917be78d5fe982dea9e33ce04ec227b662/`.

The archive contains the matching merged/application images, ELF, MAP, bootloader,
partition table, flash arguments and manifest. It is ignored by Git and has not
been published. Verify with `python3 tools/archive_firmware.py verify` followed
by the archive path. The firmware uses the original BSP and unchanged partition
layout; no Xiaozhi SDK was used.

## Actual validation

`./tools/validate.sh` completed with ESP-IDF 5.5.3; log:
`build/full-gate-layout-direct-next.log`.

`tests/test_zumi.c` also passed with AddressSanitizer and UndefinedBehaviorSanitizer.
It checks all six solvable levels, a reciprocal-edge oracle, fixed tiles, 3,000 ms
wake timing, key suppression, all eight wake-card selections without adjacent
repeats, fade boundaries, Garden placeholder and audio event/PCM bounds.
`node tests/wasm_parity.cjs` compares 5,000 transitions including idle/wake cycles.
`tests/test_zumi_artifacts.py` validates source/module hashes, eight distinct
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
the character with a preview of the selected board. All six previews were
checked, with 28–43 drawing commands below the 64-command limit and no new images.
After a win, OK starts the next board directly, wrapping after level six; there
is no inter-level picker. Initial Play entry still allows level selection.
Changing levels cancels previous game audio. Screenshots:
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
