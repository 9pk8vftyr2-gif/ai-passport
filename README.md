[简体中文](README.zh_CN.md)

# Zumi Pocket

An offline, three-button pocket companion for FoloToy AI Passport (ESP32-C3,
240 x 320 portrait display, 8 MB Flash, no PSRAM). Zumi is a boy; this prototype
uses ZCO's existing ivory 2D character master, with small face-only expression
derivatives. His appearance has not been redesigned.

## Try the browser prototype

From the repository root:

```sh
python3 -m http.server 8876 --bind 127.0.0.1
```

Open <http://127.0.0.1:8876/preview/>. The checked-in WebAssembly module needs no
package install to preview. Use the three buttons below the display or keyboard
Up / Down / Enter. Hold OK / Enter for 500 ms to return home. The screen itself
has no touch controls. Keep the local server running; no external service is
needed. Gameplay continues offline after the page's local resources load.

## Phase one

- **Home:** Talk / Play / Garden. Zumi keeps his original appearance.
- **Energy Link:** the only game, adapted from the website connection puzzle.
  Six pocket-authored 3 x 3 boards. UP/DOWN cycle through movable wires;
  OK rotates clockwise. `Solar power` / `Wind power` name the fixed source; `House` names each home. Matching
  connectors carry power; light every home to win. No countdown or move limit.
  The completed board stays visible. OK opens the next level; hold OK goes home.
  On the start screen, UP/DOWN select any of six levels.
- **Sound:** website Energy Link rotation, connection and victory tones, played
  from the same 16 kHz PCM in the browser and firmware. Audio failure leaves the
  puzzle playable; physical speaker loudness/timbre await device testing.
- **Wake greeting:** menus dim after 60 seconds. A key press restores brightness,
  chooses a different existing expression, and consumes the wake press. A
  3-second greeting fades back to home, with `zco.eco` in the corner. Games stay lit.
- **Tiny Actions (Garden):** eight offline action cards. UP/DOWN browse, OK marks
  a card done. Completed cards show Zumi cheering; OK advances to an unfinished
  card. Each card counts once, and all eight completed unlock an explicit new set.
  Progress stays during this boot, including game/menu/wake transitions.
- **Talk / Garden services:** voice remains unavailable; Garden networking is
  still an interface stub. Local cards do not sync or measure real-world actions.
- Motion detection and the one-hour standing reminder are deferred until hardware
  support is confirmed, as requested. No inactivity substitute is enabled.
- Session state resets on reboot. No storage, recording, network or paid AI.
  This is a puzzle, not real power simulation or carbon accounting.

The firmware and browser compile the **same C game logic and draw list**. The
browser paints with Canvas; firmware paints with LVGL through the original BSP.
Browser rendering is a behavioral preview, not proof of LCD appearance or timing.
The baseline hardware-test sources remain reference material but are not linked
into this application's entry point.

## Development and validation

Use ESP-IDF **5.5.3** from this repository's main baseline; do not use a different
SDK requirement from a Xiaozhi port. Install the five maintained skills with
`python3 tools/install_passport_skills.py --install` and check them with `--check`.

```sh
. /path/to/esp-idf-v5.5.3/export.sh
idf.py --version
./tools/validate.sh
```

For a host-only gate use `./tools/validate.sh --static`. For a firmware-only gate
use `./tools/validate.sh --firmware`. Neither command flashes a device. A successful
build preserves a verified merged image and matching ELF/MAP bundle under
`build/firmware/`; the app-only image is not a full-device image.

To rebuild the browser module, install wasi-sdk 34 and run:

```sh
WASI_SDK_PATH=/path/to/wasi-sdk-34 ./tools/build_preview.sh
node tests/wasm_parity.cjs
```

For authoring/test dependencies, use `pnpm install --frozen-lockfile` (Node.js required).

The parity test compiles a native C replay and compares 5,000 transitions with
WebAssembly. With Playwright and Chrome installed, run
`node tests/browser_zumi.cjs` while the preview server is running. Set
`PLAYWRIGHT_CHANNEL` if using another Playwright Chromium channel. Screenshots and
machine-readable results are written under ignored `build/`.

The resource compiler is `node tools/build_zumi_assets.cjs` (requires Sharp).
Source artwork and provenance are in `assets/images/zumi-pocket/`; generated C
sprites are checked in so firmware builds do not require Node or Sharp.

See [design and hardware acceptance](docs/assets/zumi-pocket.md) and
[validation report](docs/assets/zumi-pocket-validation.md).

## Branch boundary

Fork: `9pk8vftyr2-gif/ai-passport`; branch: `feature/zumi-pocket`.
Based on upstream main `3d685a90bdb96dd8526d84edde1e63fec25b460e`.
Upstream and fork main stay unchanged. This project does not modify the ZCO
website, deploy to Cloudflare, flash devices, erase storage, or connect paid AI.
