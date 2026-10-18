[简体中文](zumi-talk-v1-validation.zh_CN.md)

# Talk V1.0 review validation

2026-10-18. Review-stage scope: [architecture and budget](zumi-talk-v1.md),
independent C state model, WASM browser UI, and regression of the existing Zumi
firmware. No production Talk audio/network adapter is implemented or claimed.

| Field | Result | Actual evidence |
| --- | --- | --- |
| Build | PASS | Complete official gate under ESP-IDF 5.5.3; existing Zumi firmware rebuilt, merged layout and matching ELF verified. Talk model separately compiled as native C and WASM, **not linked into device firmware** |
| Host Tests | PASS | Official repository/BSP suite and existing 100,000-transition game tests; Talk six-state tests, 100,000 model transitions under ASan/UBSan, cancellation and timeout boundaries, stale turns, bounded UTF-8 text; 10,000 Talk native/WASM comparisons; existing 5,000 game comparisons |
| Browser Demo | PASS | Chrome desktop and 390 x 844 viewport; six states, key start/commit/interrupt, 500 ms long OK, recovery, scripted ASR/answer/completion, offline local interaction, no microphone request or external network request; existing Zumi browser regression also passes |
| Device Tests | NOT RUN | No serial/device access, flashing, erase or reset |
| Unverified | Pending | Runtime free/largest/minimum Heap, TLS peak, real recording/ASR/LLM/TTS, transport parser/queues/drain, audio arbitration, cancellation latency, provisioning/authentication, CJK font coverage, LCD readability, power/thermal/endurance |

## Rebuilt artifact identity

The full gate ran on `b61f8c0` plus this review increment. Runtime sources and
`main/CMakeLists.txt` are unchanged; model files are deliberately not listed there.
The embedded version is `b61f8c0-dirty`. Application size remains 2,006,896 bytes,
merged size 2,072,432 bytes, with 6,316,176 bytes left in the factory partition.
Build identity changes do not represent Talk transport implementation.

- Full-image SHA-256: `d3a3dee76e75f8b992bc9ec187cf076e114b7d0b05b9deeefbeef2728dd8a6d3`.
- Matching ELF SHA-256: `09563264221de96301e5c682da9a8d088cf664db2d0bf5690d8f21e65341d667`.
- Verified local archive: `build/firmware/d3a3dee76e75f8b992bc9ec187cf076e114b7d0b05b9deeefbeef2728dd8a6d3/`.
- Log: `build/full-gate-talk-review.log`.

The archive is Git-ignored and has not been uploaded. The architecture audit
retains the prior verified image identity to make the before/after comparison
explicit. Corrected the prior documentation's LCD DMA count from 9,600 to 19,200
bytes by inspecting the actual 40-row BSP source; no display-driver edit was made.

## Reproduce software checks

```sh
# From an activated ESP-IDF 5.5.3 environment
./tools/validate.sh
# Native review model with memory/undefined-behavior instrumentation
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Imain/zumi \
  tests/test_talk_model.c main/zumi/talk_model.c -o build/test_talk_model
build/test_talk_model
# Set WASI_SDK_PATH to installed wasi-sdk 34 first
bash tools/build_talk_preview.sh
node tests/talk_parity.cjs
node tests/wasm_parity.cjs
# With local server on 8876 and Playwright + Chrome available
node tests/browser_talk.cjs
node tests/browser_zumi.cjs
```

Preview assets are served by the existing repo-root server on port 8876. If it
is stopped, run `python3 -m http.server 8876 --bind 127.0.0.1` from the repo root,
then open `/preview/talk/`. No deployment is required.

The audit can be reproduced with `tools/audit_talk.py --archive <verified-bundle>
--size-report <idf-size-json>`. Obtain the JSON using the installed IDF
`tools/idf_size.py --format json2 <matching-map>`; the audit checks the archive,
source hashes, partition math and actual display-buffer definition. It deliberately
emits null runtime Heap values. `tests/talk_parity.cjs` verifies that audited
runtime source and browser-model source hashes still match.

Native model checks include exact 15 s recording, 20 s processing, 5 s audio-gap,
60 s total speaking and 10 s reconnect limits; reconnect/retry never starts the
microphone, playback never overlaps recording, cancelled turns cannot reactivate
playback, error/cancel clears text, and text caps preserve complete UTF-8 codepoints.
Malformed wire messages and actual ring buffers are outside this model's scope.

## Six-state visual evidence

These are browser screenshots of the review UI, not photographs of the device.
The original Zumi sprites are reused; no new character art was generated.

| Idle | Listening | Processing |
| --- | --- | --- |
| ![Idle](talk-v1/idle.png) | ![Listening](talk-v1/listening.png) | ![Processing](talk-v1/processing.png) |

| Speaking | Offline | Error |
| --- | --- | --- |
| ![Speaking](talk-v1/speaking.png) | ![Offline](talk-v1/offline.png) | ![Error](talk-v1/error.png) |

Browser results: `build/talk-browser-results.json`, `build/talk-mobile.png`;
existing-game regression: `build/browser-results.json`. Speaking is a scripted
playback-state simulation without actual TTS audio. Browser fonts are not proof
of LVGL glyph support. No provider, TLS endpoint or microphone was contacted.

## Device acceptance after implementation and explicit flash approval

- Log startup/free/minimum/largest internal Heap, DMA availability and every worker
  stack watermark at each architecture-defined phase; capture TLS peaks, not only
  steady state. Apply provisional memory gates and investigate monotonic loss.
- Confirm one audio owner, no mic reads/uploads outside Listening, no unwanted
  restart after wake/reconnect/retry, and no history or audio writes to Flash.
- Measure recording gain/nonzero samples, correct 16 kHz playback, voice clarity,
  jitter and time from button interrupt to silence. Verify bounded worker join
  and avoid freeing or reopening resources while a blocked driver still uses them.
- Test Wi-Fi setup/wrong password/no AP, DNS failure, invalid certificate, expired
  device token, service rejection, malformed/oversized packets, missing sequence,
  full queues, disconnect during each active state and delayed cancelled replies.
- Verify final queued speech finishes before DONE, 15-second capture cap, automatic
  timeout, and no duplicate commit/replay on reconnect. Review provider retention.
- Exercise actual three-key timing, long-release suppression, screen clipping and
  CJK coverage if selected. Validate original character and wake-card appearance.
- Repeat 100 session cycles and 30 minutes of Talk/game alternation; preserve
  six 4 x 4 puzzles, sound, wake-card 3-second resume and 60-second idle screen-off.
- Measure whole-device current in Idle, recording, TLS/network, speaking and
  screen-off; do not infer battery life from Flash/Heap figures.

These boxes remain unperformed. No device or paid-service approval is requested
as a prerequisite to reviewing the delivered design.
