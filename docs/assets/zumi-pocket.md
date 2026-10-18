[简体中文](zumi-pocket.zh_CN.md)

# Zumi Pocket: Design and Acceptance

## Accepted boundary and conservative defaults

Phase one is an offline software prototype before hardware arrival. The device
UI uses English ASCII strings with the configured Montserrat fonts; browser
explanations are Chinese. This avoids claiming unverified CJK glyph support.
Three physical keys are the only input. UP/DOWN move selection on PRESS;
home selection clamps, levels and movable wires wrap. OK acts on RELEASE;
LONG at 500 ms returns home and suppresses that release. CLICK/DOUBLE are ignored.

Energy Link is the only game. Six authored 3 x 3 boards preserve the website's
reciprocal N/E/S/W connectors and source-to-all-homes win condition. Unused wires
are allowed. Fixed source/home tiles and marked fixed clues cannot rotate.
There is no time or move limit. Completion keeps the board visible; OK advances
to the next level's start screen, wrapping after six. Any level can be selected.
The website's 5 x 5 boards, generated Hexapipes levels, artwork, undo/history,
sound and persistence are not imported. Rules were inspected read-only at
website source revision `a0504f97fb74a94ea864361f539e5832a16b268f`,
`game/energy-link/engine.mjs` and `levels.mjs`. Pocket levels and C code are new.

Menus dim after 60 seconds (70% to 20% brightness). A PRESS restores brightness
and randomly selects a different one of the three existing expressions. The
wake press and any presses during the greeting are consumed through RELEASE.
The greeting occupies 3,000 ms: 240 ms fade-in, hold to 2,640 ms, 360 ms fade-out;
home then fades in for 240 ms. Each expression view has a `zco.eco` corner label.
The firmware fades individual objects, without a full-screen compositing buffer.
Games remain lit while thinking. This is not deep sleep or verified low power.
Battery reads use the BSP and show `--` on failure. Talk/Garden perform no I/O.

## Architecture and budget

- Allocation-free deterministic C model: 76 bytes, no timer-driven game state.
  Updates are capped at 100 ms after suspension. Prototype randomness uses a
  repeatable xorshift seed; it is not a claim of unpredictable randomness.
- Shared view: at most 64 commands, 6,404 bytes per list; two firmware lists use
  12,808 bytes. LVGL objects are reused; changed commands update under BSP locks.
- One owner task, nonblocking bounded input queue; battery I/O outside LVGL lock.
- Three 78 x 92 RGB565 sprites occupy 43,056 Flash bytes. No full-screen decoded
  images. BSP partial DMA buffer is 9,600 bytes; LVGL pool remains 24 KB.
  Peak runtime heap, LVGL allocation and fragmentation need physical validation.
- Original BSP and 8 MB partition table unchanged; no PSRAM. Enabled Montserrat
  10 for small website labels alongside existing 14/20 fonts. Original test UI
  sources stay unlinked. No NVS persistence, networking, audio or sleep.
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
   rounded corners, RGB565 colors, text clipping and ZCO cap mark. Approve the
   three small expressions and compare against the original art.
3. Exercise rapid taps, all three keys, level/wire wrapping and fixed-tile skipping.
   Verify long OK and its release never open Talk or rotate an extra wire.
4. Complete all six Energy Link boards, including branching and fixed clues.
   Verify only reciprocal connectors conduct, every home is required, completed
   boards remain visible, next/replay works and long OK exits.
5. Wait 60 seconds on each menu, wake with each key, confirm a different
   expression, `zco.eco`, three-second greeting and smooth transition home.
   Hold the wake key beyond the greeting and verify no accidental action.
6. Disconnect networking: the game still works and Talk/Garden stay unavailable.
7. Run 30 minutes and 100 page switches. Record minimum free heap, largest free
   block, LVGL pool peak, watchdog behavior, input latency and frame rate.
8. Check battery gauge failure, real battery readings, idle/game current and
   temperature. Battery life and fade smoothness remain unverified on hardware.
9. Power-cycle only after authorization; verify session progress resets and the
   application never writes or erases unrelated storage.
10. Record board revision, image hash and evidence for each item before assigning
    Device Tests PASS/FAIL. Browser screenshots cannot replace this checklist.
