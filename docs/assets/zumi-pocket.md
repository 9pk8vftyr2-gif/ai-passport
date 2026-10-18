[简体中文](zumi-pocket.zh_CN.md)

> Historical stage record; see the [local integration report](zumi-talk-integration.md) for current Talk behavior.

# Zumi Pocket: Design and Acceptance

The home screen uses three numbered Talk / Play / Garden rows with a short
purpose line and a dark selected row. No character image appears there.
Energy Link's level picker shows the selected level's initial 4 x 4 board,
using the same wire geometry and full energy labels as gameplay, without a
character image. UP/DOWN changes the preview; OK starts that board. Wake-card
art and the Talk/Garden placeholder characters are unchanged.


## Accepted boundary and conservative defaults

Phase one is an offline software prototype before hardware arrival. The device
UI uses English ASCII strings with the configured Montserrat fonts; browser
explanations are Chinese. This avoids claiming unverified CJK glyph support.
Three physical keys are the only input. UP/DOWN move selection on PRESS;
home selection clamps, levels and movable wires wrap. OK acts on RELEASE;
LONG at 500 ms returns home and suppresses that release. CLICK/DOUBLE are ignored.

The in-game level/turn counter sits at (22,47), above the four-row board
starting at y=72. Cells are 48 x 44 pixels; the initial picker uses 48 x 34
pixel cells. All six solutions use the fourth row.

Energy Link is the only game. Six authored 4 x 4 boards preserve the website's
reciprocal N/E/S/W connectors and source-to-all-homes win condition. Unused wires
are allowed. Fixed source/home tiles and marked fixed clues cannot rotate.
There is no time or move limit. Completion keeps the board visible; OK advances
directly to the next board without an inter-level screen, wrapping after six.
The initial Play entry still offers level selection.
The website's 5 x 5 boards, generated Hexapipes levels, undo/history,
and persistence are not imported; synthesized sound parameters are reused. Rules were inspected read-only at
website source revision `a0504f97fb74a94ea864361f539e5832a16b268f`,
`game/energy-link/engine.mjs` and `levels.mjs`. Pocket levels and C code are new.

All pages, including Energy Link, switch the backlight from 70% to 0% after
60 seconds without input. Application redraws stop while the screen is off.
A PRESS restores 70% brightness
and randomly selects one of eight approved promotional cards, avoiding an immediate repeat. The
wake press and any presses during the greeting are consumed through RELEASE.
The greeting occupies 3,000 ms: 240 ms fade-in, hold to 2,640 ms, 360 ms fade-out;
the original game (when woken from Energy Link) or home fades in for 240 ms.
Game level, wires, moves, cursor and completion state stay in RAM throughout the
greeting. Resuming does not replay game sounds or advance a completed level. Each expression view has a `zco.eco` corner label.
Only the wake greeting displays “Zumi Pocket”, above the character; other
screens omit this brand heading.
The firmware fades individual objects, without a full-screen compositing buffer.
Any input restarts the inactivity timer. The three-second card does not count
toward the next inactivity timeout. Late release/long events cannot wake the display.
The application prepares the wake frame before restoring the backlight. CPU,
button polling and initialized audio resources remain active: this is reversible
backlight-off standby, not deep sleep or a measured battery-life claim.
Battery reads use the BSP and show `--` on failure. Talk/Garden services perform no I/O.

## Offline promotional wake cards

Eight cards reuse distinct original Zumi: Office Vibes sticker expressions/actions:
wave, walk, thumbs-up, relaxed eyes, conversation, close inspection, applause,
and thanks. Background and copy follow the approved V3 design. Zumi Pocket and
zco.eco are baked into each image; the battery overlay stays visible during fades.
The earlier Tiny Actions checklist is removed. Garden is a reserved service
placeholder with OK returning home; Talk is also unavailable. No completion
tracking or storage writes. Startup still enters home; cards appear on screen-off
key wake only. Random selection does not repeat the previous card; it does not
promise a shuffle bag or equal frequency over a short session.

Approved composite sources and per-sticker provenance hashes are kept under
`assets/images/zumi-pocket/wake-cards/source/`. They come from the owner's local
Office Vibes package, not a claimed verified WeChat publication. Convert with
`node tools/build_zumi_wake_cards.cjs`. Eight 240 x 320 little-endian RGB565
images occupy 1,228,800 bytes in read-only Flash. Browser PNGs are quantized to
the same RGB565 colors. Native images use image opacity with the original BSP's
partial draw buffer, avoiding an object-opacity intermediate layer and runtime
full-frame PNG/JPEG decoding. Physical frame timing still requires validation.

The official hardware guide/BSP exposed no motion sensor interface during
inspection. The user chose to defer both motion detection and the one-hour
standing reminder until hardware is confirmed. No motion detector, hourly timer,
or no-button-activity substitute is enabled in this version.

## Readable labels and shared sound

Source and house cells reuse Eco Pop's sun, light-blue turbine and orange
building artwork. The footer names `Solar power > House` or `Wind power > House`
in full. Wind appears in levels 3 and 5; source kind does not alter puzzle rules.
The exact original atlas/crops from website revision
`a0504f97fb74a94ea864361f539e5832a16b268f` are recorded in
`assets/images/zumi-pocket/energy-icons/manifest.json`; building/demand artwork
is used for House at the user's request. The two original PNGs remain under
`source/`, with no new redistribution license implied. The converter
`node tools/build_energy_link_icons.cjs` creates six 32 x 32 RGB565 lit/unlit
variants (12,288 Flash bytes); previews draw them at 28 x 28. Only the small
variants are linked into firmware. Browser PNGs use the same quantized colors.

The six 4 x 4 boards have 2, 2, 3, 4, 4 and 5 houses. Exhaustive orientation
search in `tests/zumi_difficulty.c`, with independent bit-mask connectivity,
proves minimum clockwise-turn counts of **8, 13, 16, 18, 20 and 25**. Selection
presses are excluded. These are objective puzzle costs, not a measured human
completion time. The host gate rejects non-increasing minimum turn counts.
There is no time limit or failure penalty.

Sound parameters come from the same website revision's `game.mjs`: sine waves,
12 ms exponential attack from 0.0001 to 0.045 gain, decay to 0.0001, 20 ms tail.
Rotation: 330 Hz / 70 ms. Additional home powered: 659.25 Hz / 70 ms. Victory:
a connection tone immediately, then after 620 ms the 523.25 / 659.25 / 783.99 Hz
chord with durations 180 / 180 / 250 ms and offsets 0 / 100 / 200 ms.
`assets/music/zumi-link-tones.json` records these parameters;
`python3 tools/build_zumi_sounds.py` generates the PCM without external media.

Three const mono 16-bit / 16 kHz clips occupy 20,800 Flash bytes. The shared
C event classifier distinguishes rotation, added homes and first victory.
Browser AudioContext unlocks on a user gesture and plays the same PCM as the
board. Hidden/blurred pages stop audio. A new sound replaces an older one to
bound rapid-input backlog; leaving the game cancels pending victory audio.
Firmware uses one lifetime worker with no UI pointers, a one-entry overwrite
queue, 4,096-byte stack and a 512-byte PCM chunk on that stack. Only the worker
calls BSP audio; initialization is lazy, writes block outside button/UI tasks,
and failures leave gameplay available. Cancellation is checked each chunk;
already-buffered BSP DMA can finish (up to roughly 90 ms). Hardware volume is
60% as an initial setting, not a claim of matching physical loudness.
Audio hardware resources remain initialized after first use; power consumption
and display/audio contention require device measurement. No microphone reads.

## Architecture and budget

- Allocation-free deterministic C model: 104 bytes, no timer-driven game state.
  Updates are capped at 100 ms after suspension. Host/browser tests use a
  repeatable xorshift seed; it is not a claim of unpredictable randomness.
- Shared view: at most 64 commands, 6,404 bytes per list; two firmware lists use
  12,808 bytes. LVGL objects are reused; changed commands update under BSP locks.
- One owner task, nonblocking bounded input queue; battery I/O outside LVGL lock.
- Three 78 x 92 RGB565 sprites occupy 43,056 Flash bytes. No full-screen decoded
  images. BSP partial DMA buffer is 19,200 bytes; LVGL pool remains 24 KB.
  Peak runtime heap, LVGL allocation and fragmentation need physical validation.
- Original BSP and 8 MB partition table unchanged; no PSRAM. Enabled Montserrat
  10 for small website labels alongside existing 14/20 fonts. Original test UI
  sources stay unlinked. No NVS persistence, networking, recording or sleep.
- Browser uses identical C model/view compiled to WASM and draws with Canvas.
  Browser font rasterization and timing do not prove device behavior.

## Reused official patterns

Reviewed upstream main at `3d685a90bdb96dd8526d84edde1e63fec25b460e` and:

| Branch | Tip | Applied idea |
| --- | --- | --- |
| demo/stopwatch | 7807feb1fabbf7e269597df29bcc298b4f7cb56a | Pure C model and host tests |
| demo/tetris-game | f2f6693b1b962bc3f9ab3f854b9598e8eb002372 | Low-latency PRESS input, bounded event handling and small redraws |
| demo/rock-paper-scissors | faed510e2bf0d1c92ee084476b3e21be5dbd4ce4 | Flash-resident RGB565 assets |

The application has its own home, level selector, puzzle board, completion feedback and
service placeholders. No baseline test menu, test-screen shell or `ui_pixel`
widgets are used. The required five passport skills are repository-local links
under `.agents/skills/`, verified by the official installer and readable through
those paths. They are generated/ignored and recreated per checkout.

## Character provenance

Zumi is a boy. `assets/images/zumi-pocket/zumi-original.svg` is an unchanged copy
of the existing `ZUMI_2D_FRONT_IVORY_v1.1.svg` master-derived vector. The source
hash and generated-image hashes are in the adjacent `manifest.json`. Preserve
his earth silhouette, blue ocean, light-green continents, green limbs, ivory
cap/shoes and plain ZCO wordmark. Focus/cheer change only face expression paths;
the original stays intact. These small derivatives are prototype artwork and
still need the owner's visual acceptance. ZCO artwork does not grant a new
third-party character license; upstream code retains its existing MIT license.

## Hardware arrival checklist — NOT RUN

No device has been flashed, reset, erased or opened during this phase. Any future
flashing requires a new explicit authorization and a chosen target and image.

1. Confirm AI Passport ESP32-C3 board/revision, 8 MB Flash, a data-capable USB cable
   and the exact successful firmware manifest/hash before any approved write.
   Agree on preservation of existing NVS data: merged images can reset it.
2. Confirm boot goes directly to Zumi's home. Inspect 240 x 320 portrait,
   rounded corners, RGB565 colors and text clipping. Confirm no character on home
   or level selection, and a matching board preview for all six levels. Inspect
   the ZCO cap mark on retained character pages. Approve the
   small expressions and all eight wake cards against the approved art.
3. Exercise rapid taps, all three keys, level/wire wrapping and fixed-tile skipping.
   Verify long OK and its release never open Talk or rotate an extra wire.
4. Complete all six Energy Link boards, including branching and fixed clues.
   Verify only reciprocal connectors conduct, every home is required, completed
   boards remain visible, next/replay works and long OK exits.
5. Wait 60 seconds on every page, including a puzzle, and confirm the backlight
   is fully off. Input before the deadline must restart the timer. Wake with
   each key, confirm a different
   card, Zumi Pocket, `zco.eco`, three-second greeting and smooth transition to the original game or home.
   Verify game level, wires, moves, cursor and completion state survive repeated
   wakes; a new OK press continues playing or advances a completed level.
   Hold the wake key beyond the greeting and verify no accidental action.
6. Wake repeatedly offline, inspect all eight distinct cards and no adjacent
   repeats. Confirm no Tiny Actions checklist, one-hour reminder or motion
   detection. Talk/Garden stay unavailable; games and wake cards need no network.
7. Run 30 minutes and 100 page switches. Record minimum free heap, largest free
   block, LVGL pool peak, watchdog behavior, input latency and frame rate.
8. Audition rotation, connection and victory sounds against the website. Check
   speaker loudness/distortion, cancellation on exit, rapid presses, unavailable
   codec and simultaneous drawing/audio. Check battery gauge failure, real battery readings, lit/screen-off/game current and
   temperature. Battery life and fade smoothness remain unverified on hardware.
9. Power-cycle only after authorization; verify session progress resets and the
   application never writes or erases unrelated storage.
10. Record board revision, image hash and evidence for each item before assigning
    Device Tests PASS/FAIL. Browser screenshots cannot replace this checklist.

Firmware seeds the non-security PRNG with esp_random(); tests use fixed seeds.
Both Energy Link titles are at (22,16), clear of the battery reading.
