[简体中文](zumi-pocket-validation.zh_CN.md)

# Zumi Pocket Validation

2026-10-18. Firmware source: `6a95860` on `feature/zumi-pocket`.
Scope: phase-one software prototype before hardware arrival.

| Field | Status | Evidence |
| --- | --- | --- |
| Build | PASS | Full official gate, ESP-IDF v5.5.3, ESP32-C3; merged image and matching ELF verified |
| Host Tests | PASS | Repository static/baseline checks; six solvable levels; eight-card completion/deduplication/reset/session retention; 100,000 transitions with independent connectivity oracle; sound events, PCM gain/bounds and victory delay; AddressSanitizer/UndefinedBehaviorSanitizer; 5,000 native/WASM replay comparisons |
| Browser Demo | PASS | Chrome desktop 1440 x 1000 and mobile viewport 390 x 844; 23 interaction checks; offline play; no page errors; screenshots visually inspected |
| Device Tests | NOT RUN | Hardware unavailable; no device flashing, erase or reset |
| Unverified | OPEN | Physical speaker loudness/timbre/distortion, audio-failure recovery and concurrent display/audio, real LCD/font appearance, fade smoothness, input latency, LVGL allocation peak, long-run fragmentation, battery/current/thermal measurements, Safari, physical-phone use and user visual acceptance |

## Firmware evidence

Application: 765,456 bytes in an 8,323,072-byte factory partition. Merged image:
830,992 bytes. Original BSP and partition table unchanged. No PSRAM dependency.
Enabled Montserrat 10 for the website label. No stored-data format or writes added. Audio uses the original BSP with a
  dedicated chunked worker; website-matched PCM occupies 20,800 Flash bytes.

Merged SHA-256:
`5048a5a3749b3f9f9a80d355250cdd03f5f536f27ea2221eeaecad0da5757e7e`.
Matching ELF SHA-256:
`0a725f0edd0f7fe493e05ba4976dd214d72a0ca0ed04214127df42988c1839ae`.

The local image/ELF/MAP/manifest bundle is in
`build/firmware/5048a5a3749b3f9f9a80d355250cdd03f5f536f27ea2221eeaecad0da5757e7e/`.
Its app descriptor confirms source version `6a95860` and IDF `v5.5.3`.
Build outputs are ignored, not published firmware releases. The final report-only
commit does not change the tested firmware, browser module or assets.

## Reproduce and test scope

- Activate ESP-IDF 5.5.3 and run `./tools/validate.sh`. Local evidence:
  `build/full-gate-cards.log`. All original baseline checks remain enabled.
- `cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Imain/zumi tests/test_zumi.c main/zumi/core.c main/zumi/view.c main/zumi/sound.c main/zumi/sounds.c -o build/test_zumi`
  then `build/test_zumi`: solve all six levels via actions; verify fixed tiles,
  untimed play, next-level wrapping, all wake keys, consumed release, exactly
  3,000 ms greeting, home fade, URL and draw bounds. Randomized states are checked
  against an independent reciprocal-edge connectivity traversal.
- `node tests/wasm_parity.cjs`: 5,000 native C/browser WASM state comparisons.
  Source/module hashes guard against stale generated modules.
- With the preview server running, `node tests/browser_zumi.cjs`: home, Talk,
  Garden, game entry, first-level completion, next level, offline rotation,
  untimed thinking, long-OK release, idle dim, expression change, corner URL,
  three-second return, mobile overflow, physical-button controls, full English
  labels, real Web Audio PCM playback, sound cancellation on exit, card browsing,
  offline completion of eight cards, deduplication, retention across home and
  explicit new-set reset.
- Browser evidence: `build/browser-results.json`, `browser-home.png`,
  `browser-link.png`, `browser-link-complete.png`, `browser-wake.png` and
  `browser-mobile.png`, `browser-wind.png`, `browser-actions.png`,
  `browser-action-done.png`, `browser-actions-complete.png`,
  `browser-actions-mobile.png` under `build/`. A mobile viewport is not a phone test.
- Artwork compilation preserves source hashes; no image generation or paid
  service was called. Browser rendering cannot validate physical LVGL memory.

The official gate required two macOS portability fixes: use `shasum` for the
pinned actionlint checksum and Darwin `-dead_strip` in existing host demo tests;
Linux retains `--gc-sections`. No tests were removed or suppressed.

## Environment and delivery boundary

Five official passport skills installed and readable. ESP-IDF v5.5.3 tag object
`b31fcc7a314a44ad992b58f589f7d1d8a4fadff6`, source commit
`2c211b236707889e8400c4dc5644dd5c4ee071e0`. No Xiaozhi SDK mixing.
wasi-sdk 34 builds the browser only. Character artwork provenance is recorded
in `assets/images/zumi-pocket/manifest.json`.

Independent checkout and fork `9pk8vftyr2-gif/ai-passport`, feature branch only.
Upstream and fork main both verified at `3d685a90bdb96dd8526d84edde1e63fec25b460e`.
The ZCO website was read only for Energy Link rules; no site modification,
Cloudflare deployment, paid AI connection or device operation was performed.
Only action cards were added in this increment. Motion detection and the
one-hour standing reminder are deferred by user choice until hardware is
confirmed. No hourly or button-inactivity substitute is enabled. Card completion
is self-reported and retained within this boot only; no cloud/storage writes.
See the [hardware-arrival checklist](zumi-pocket.md#hardware-arrival-checklist--not-run).
