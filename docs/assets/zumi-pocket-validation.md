[简体中文](zumi-pocket-validation.zh_CN.md)

# Zumi Pocket Validation

2026-10-18. Firmware source content committed as `e0ea6d6` on
`feature/zumi-pocket`. The successful gate ran before that commit, so the image
version is `7753a12-dirty`; no firmware source or display pixel changes followed
that build. This report and binary Git attributes do not change firmware.

| Field | Status | Evidence |
| --- | --- | --- |
| Build | PASS | ESP-IDF 5.5.3, complete official gate, merged layout and matching ELF archive verified |
| Host Tests | PASS | Original repository checks, sanitizers, 100,000 game transitions, 256 wake cycles and all eight cards, 5,000 native/WASM comparisons, provenance and pixel-data hashes |
| Browser Demo | PASS | Chrome desktop/mobile; eight distinct cards offline, no adjacent repeats, three-second return, game controls, full English labels and shared sound |
| Device Tests | NOT RUN | No physical device flashed, erased, reset or opened |
| Unverified | Pending hardware | LCD colors and clipping, fade frame time, internal heap/LVGL peak, audio, battery current and real runtime |

## Exact firmware identity

- Application: 1,993,584 bytes of the 8,323,072-byte factory partition.
- Merged image: 2,059,120 bytes. Flash configuration: 8 MB; no PSRAM.
- Eight wake cards: 1,228,800 read-only Flash bytes, native 240 x 320 RGB565.
- Basic sprites: 43,056 Flash bytes; shared audio PCM: 20,800 bytes.
- State: 84 bytes; two draw lists: 12,808 bytes. Original 9,600-byte BSP draw
  buffer and 24 KB LVGL pool retained. No added full-frame decoder buffer.
- Full-image SHA-256: `9299330935d98dbc80234ec55945692250cacf0dd26e61abb92d445fa3682fef`.
- Matching ELF SHA-256: `6c6bdbc7419ad0797b4cfcf66f3969dfd155bba9edf95c9dff453bb9835ef8d2`.
- Local bundle: `build/firmware/9299330935d98dbc80234ec55945692250cacf0dd26e61abb92d445fa3682fef/`.

The archive contains the matching merged/application images, ELF, MAP, bootloader,
partition table, flash arguments and manifest. It is ignored by Git and has not
been published. Verify with `python3 tools/archive_firmware.py verify` followed
by the archive path. The firmware uses the original BSP and unchanged partition
layout; no Xiaozhi SDK was used.

## Actual validation

`./tools/validate.sh` completed with ESP-IDF 5.5.3; log:
`build/full-gate-wake-cards.log`. The initial shell selected Python 3.9, and a
later CMake wrapper lacked its Python package. Selecting the existing IDF Python
3.12 environment and direct CMake binary resolved both; the final complete gate
passed without disabling checks or changing the toolchain version.

`tests/test_zumi.c` also passed with AddressSanitizer and UndefinedBehaviorSanitizer.
It checks all six solvable levels, a reciprocal-edge oracle, fixed tiles, 3,000 ms
wake timing, key suppression, all eight wake-card selections without adjacent
repeats, fade boundaries, Garden placeholder and audio event/PCM bounds.
`node tests/wasm_parity.cjs` compares 5,000 transitions including idle/wake cycles.
`tests/test_zumi_artifacts.py` validates source/module hashes, eight distinct
original sticker hashes, approved source hashes, PNG dimensions and RGB565 slices.

`node tests/browser_zumi.cjs` passed after loading local assets and switching the
browser offline. Evidence: `build/browser-results.json`, `browser-play-title.png`,
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
Energy Link titles now sit at (22,16), clear of battery information.

Menus still dim after 60 seconds and wake cards return home after three seconds.
This is not deep sleep. The official 500 mAh capacity alone cannot establish
runtime: average board current, brightness and standby behavior require
measurement. No motion detector, hourly standing reminder or new sleep policy
was enabled. No ZCO website edit, Cloudflare deployment or device operation.
Fork/upstream main were both verified unchanged at
`3d685a90bdb96dd8526d84edde1e63fec25b460e`.
See the [hardware-arrival checklist](zumi-pocket.md#hardware-arrival-checklist--not-run).
