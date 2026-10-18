[简体中文](zumi-pocket.zh_CN.md)

# Zumi Pocket: Design and Acceptance

## Accepted boundary and conservative defaults

Phase one is an offline software prototype before hardware arrival. The device
UI uses English ASCII strings with the configured Montserrat fonts; browser
explanations are Chinese. This avoids claiming unverified CJK glyph support.
Three physical keys are the only required input. UP/DOWN act on PRESS and clamp
at menu/lane ends. OK acts on RELEASE; a LONG event at the BSP's 500 ms threshold
returns home and suppresses that release. Generated CLICK/DOUBLE events are
ignored so rapid presses do not double-act. No simultaneous-key chords are used.
The hardware power button remains separate.

Each game lasts up to 30 seconds, has three hearts, supports replay and early exit.
No NVS game persistence, audio, radio, deep sleep or automatic dimming is added
in phase one. Battery readings use the official BSP and show `--` on failure.
Talk and Garden return `Z_SERVICE_UNAVAILABLE` without doing any I/O.
No external AI credentials or personal data enter this project.

## Architecture and budget

- `main/zumi/core.c`: deterministic, allocation-free state machine; 20 ms fixed
  steps, capped to 100 ms per update after suspension. Xorshift seed is fixed for
  reproducible prototypes. No claim of unpredictable randomness is made.
- `main/zumi/view.c`: shared 240 x 320 drawing list, at most 48 commands.
- `main/zumi_main.c`: original BSP, a bounded nonblocking input queue, one owner
  task and LVGL locks. Reuses objects and updates changed commands. No per-page
  task or timer can retain a deleted page. Battery I/O runs outside the LVGL lock.
- `preview/bridge.c`: compiles the same model and view to WebAssembly; Canvas
  interprets the list. Browser fonts and physical LCD output are not identical.
- Model: 164 bytes; draw list: 4,612 bytes; two firmware lists: 9,224 bytes.
  Three const 78 x 92 RGB565 sprites occupy 43,056 Flash bytes. They are not
  decoded into full-screen RAM buffers. Existing BSP partial DMA buffer is
  9,600 bytes; the LVGL pool remains 24 KB. Runtime peak heap and fragmentation
  still require a device measurement.
- 8 MB partition table and BSP files remain unchanged. No PSRAM dependency.
  Original hardware-test sources stay unlinked for reference and existing tests.

## Reused official patterns

Reviewed upstream main at `3d685a90bdb96dd8526d84edde1e63fec25b460e` and:

| Branch | Tip | Applied idea |
| --- | --- | --- |
| demo/stopwatch | 7807feb1fabbf7e269597df29bcc298b4f7cb56a | Pure C model and host tests |
| demo/tetris-game | f2f6693b1b962bc3f9ab3f854b9598e8eb002372 | Low-latency PRESS input, bounded event handling and small redraws |
| demo/rock-paper-scissors | faed510e2bf0d1c92ee084476b3e21be5dbd4ce4 | Flash-resident RGB565 assets |

The application has its own home, game picker, playfields, result screens and
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
3. Exercise all three keys, rapid taps, menu/lane limits, OK short/long/release,
   and 100 successive home/game transitions. Ensure a long-press release does
   not open Talk or trigger an in-game action. Check key identity while held.
4. Carbon Pop: correct target +10, leaf/sun penalty, 1.8-second expiry, three
   hearts, timeout victory, loss, replay and early home exit.
5. Journey: all three lanes, green + collection, rock hit, jump protection,
   600 ms duration, 1,000 ms cooldown, timeout victory, loss and replay.
6. Disconnect networking and verify both games remain fully playable. Confirm
   Talk/Garden explicitly remain unavailable and do not start radio or microphone.
7. Run at least 30 minutes including 100 page switches. Record free internal
   heap, minimum free heap, largest block, LVGL pool peak and watchdog behavior.
   Measure button-to-screen latency and actual frame rate; build success is not
   timing or memory-fragmentation proof.
8. Check real battery reading and unavailable-gauge behavior. Measure idle/game
   current and temperature. Battery life and low-power behavior are unverified;
   the first prototype deliberately has no sleep policy.
9. Power-cycle after permission, verify session scores reset as documented and
   that unrelated stored data has not been erased by application logic.
10. Record board revision, firmware hash, observed result and evidence per item;
    only then assign Device Tests PASS/FAIL. Browser screenshots cannot fill this
    checklist.
