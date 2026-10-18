[简体中文](zumi-talk-v1.zh_CN.md)

> Historical stage record; see the [local integration report](zumi-talk-integration.md) for current Talk behavior.

# Zumi Talk V1.0: architecture review

Date: 2026-10-18. Review baseline: `b61f8c0` on `feature/zumi-pocket`.
This increment delivers a source/artifact audit, a memory plan, an executable
six-state C model and a local 240 x 320 browser UI. It does **not** connect a
microphone, Wi-Fi station, speech provider or TTS player. The model is deliberately
absent from `main/CMakeLists.txt`; the existing firmware Talk placeholder and
Energy Link remain unchanged. No firmware was flashed, device storage accessed,
paid service activated, or website modified.

Preview: [Talk review](http://127.0.0.1:8876/preview/talk/).
Evidence: [machine-readable audit](talk-v1/audit.json) and
[test report](zumi-talk-v1-validation.md).

## 1. Current firmware audit

Numbers below describe the verified archive, not the contents of an unseen board.

| Partition | Offset | Bytes | Purpose |
| --- | --- | ---: | --- |
| NVS | `0x9000` | 24,576 | Existing configuration space; no erase/reset planned |
| PHY | `0xf000` | 4,096 | Radio calibration data |
| factory | `0x10000` | 8,323,072 | Single application; no OTA slot or file-system partition |

- Physical configuration: 8,388,608 bytes Flash; no PSRAM. Partition table is at `0x8000`.
- Application binary: **2,006,896 bytes** (1.914 MiB).
- Remaining application capacity: **6,316,176 bytes (6.023 MiB)**, not about 3 MB.
- Merged binary: 2,072,432 bytes; its bootloader/padding must not be subtracted from the factory partition.
- Verified image: `cb681e7452c32b843511904bff3760614e3aac0f3123296854623f2f94aadd2a`.
- Matching ELF: `34e33e22cbaf63ab37d335075f6ea558b17488fb3c6d3f1003cdab5b21c16ddc`.
- Embedded version: `1c7a2cf-dirty`, ESP-IDF `v5.5.3`; this was built before the reviewed `b61f8c0` commit.
- Linker report: shared internal-memory region 321,296 bytes; used 117,484
  (67,320 resident code, 42,792 BSS, 7,372 data); remainder **203,812 bytes**.
  That remainder is **not runtime free Heap**. Startup tasks, DMA, drivers and
  allocator metadata consume more, and fragmentation affects individual allocations.
- Runtime free internal Heap, minimum free Heap and largest continuous free block:
  **UNMEASURED / null**. No board session exists. No RAM admission claim is made.

### Driver and allocation findings

| Area | Actual local source | Finding / integration implication |
| --- | --- | --- |
| ES8311 / I2S | `components/bsp/src/bsp_audio.c` | Shared BSP I2C; 16 kHz, 16-bit mono application PCM over two I2S slots; full-duplex channels exist, but Talk will use half-duplex |
| DMA | same | Six descriptors x 240 frames per direction; nominal two-slot PCM payload 5,760 bytes/direction, 11,520 combined, plus driver metadata; verify actual allocations after open |
| Blocking I/O | BSP + codec-dev I2S platform | `bsp_audio_read/write` block; bundled driver timeout is 1,000 ms. A 20 ms application chunk does not guarantee a 20 ms cancellation latency |
| Game sound | `main/zumi/audio.c` | Existing 4,096-byte worker stack, 512-byte scratch on that stack; audio initializes lazily, one-entry queue; cancellation has no completion acknowledgement yet |
| Display | `bsp_display_lvgl.c` | ST7789 SPI, LVGL 9.5; **40-row single RGB565 buffer = 19,200 bytes**, 24,576-byte static LVGL pool; prior Zumi reports incorrectly said 9,600 and are corrected in this review |
| Render state | `main/zumi_main.c` | Two 6,404-byte draw lists; application state 104 bytes; callbacks enqueue, main owner takes the LVGL lock |
| Wi-Fi | `main/CMakeLists.txt`, `main/demo_wifi.c` | No active Wi-Fi path linked into Zumi. Wi-Fi is ESP-IDF radio code, not an existing `bsp_wifi` API. Main demo only scans; STA/provisioning lifecycle is still needed |
| Assets | existing manifests | Wake cards 1,228,800, expressions 43,056, game PCM 20,800, Eco Pop icons 12,288 bytes in read-only Flash; retain all originals |
| Fonts | `sdkconfig.defaults` | Montserrat 10/14/20, no general Chinese coverage. Browser fonts do not validate device transcript rendering |

The ES8311 IN/OUT codec may leave its ADC enabled when the game opens audio.
There is currently no application microphone reader, persistent listening loop,
or speech upload. The future Talk privacy contract is stricter: no reads or
uplink outside Listening; suspend audio when unowned. Do not claim a physical
microphone power cut: the codec/amplifier supply has no software power switch.

## 2. Four repositories reviewed at pinned revisions

| Repository / revision | Reusable idea | What is excluded |
| --- | --- | --- |
| [FoloToy/ai-passport, 3d685a9](https://github.com/FoloToy/ai-passport/tree/3d685a90bdb96dd8526d84edde1e63fec25b460e) | Existing BSP ownership, codec recovery, LVGL lock and bounded audio-worker lifecycle | Hardware-test menus and full-recording demo's 96 KB allocation |
| [FoloToy/folo-ai-passport-xiaozhi, 72da544](https://github.com/FoloToy/folo-ai-passport-xiaozhi/tree/72da544d1b51678f5d88967adc299b3aec956946) | Protocol abstraction, explicit listen/stop/abort, sequenced stream events | Entire application, OTA/partitions, wakeword/AEC, SDK and default endpoints. Current README requires IDF >=6.0.1, recommends 6.1; Zumi remains 5.5.3 |
| [zhaohuaxiaoy/folo-ai-passport-voice, 403ea8e](https://github.com/zhaohuaxiaoy/folo-ai-passport-voice/tree/403ea8e244a3862e0018ef41376e32ab79062da3) | Pure C reducer, session-token invalidation, bounded capture/transport separation, drain before end | Desktop text injection, approval UI, mandatory provider, copied buffer sizes. Source uses 3,200-byte chunks and a 6,432-byte ring, plus optional ADPCM |
| [linysky/hermes-passport, 6bcf56c](https://github.com/linysky/hermes-passport/tree/6bcf56c64c27162281b2a5efe6686c7e9a8e2765) | BLE companion boundary, request/token/sequence IDs, text pagination | Multi-bot/history features. In `main/hermes_ble.c`, incoming audio/TTS is still a future placeholder; it is not a complete playback implementation |

All four top-level licenses are MIT; component licenses remain independent.
No external implementation was copied into this increment. Inspect source rather
than treating repository README capabilities as device acceptance.
Key files: XiaoZhi `main/protocols/protocol.h`, `websocket_protocol.cc` and
`main/audio/audio_service.h`; Voice `main/audio_streamer.c` and `app_protocol.h`;
Hermes `main/hermes_ble.c`, `hermes_bridge.c`, `text_layout.h` and `adpcm.h`.

Also inspected official
[demo/blufi-provisioning at 9c039cc](https://github.com/FoloToy/ai-passport/tree/9c039cc5127f22072afa83bedb7fa3d8efe635ad):
`demo_blufi.c`, `demo_blufi_security.c`, radio setup and teardown. Adapt connection
and credential handling only, retaining security callbacks. Never merge the
branch wholesale or enable a second I2C/ADC driver. Provision explicitly, do not
log Wi-Fi credentials, do not erase NVS on errors, and stop BLE before Talk to
avoid simultaneous BLE/Wi-Fi memory peaks.

## 3. Proposed architecture

```text
Three keys -> existing input queue -> Talk controller (pure state + events)
                                    |              |
                           UI snapshot       transport adapter
                                    |              |
                              existing LVGL   bounded uplink/downlink queues
                                                   |
                                             single audio owner
                                                   |
                                             BSP ES8311 / I2S

Device <---- WSS / small binary PCM frames ----> replaceable relay
                                                  | ASR adapter
                                                  | LLM adapter
                                                  | TTS adapter
```

V1 uses **16 kHz mono PCM16 little-endian**, 20 ms / 640-byte frames (32,000
bytes/s, 256 kbit/s before framing). The relay normalizes provider sample rates,
containers and text into this format. Wi-Fi avoids adding an Opus encoder/decoder
to the ESP32-C3 initially. If measured Wi-Fi/TLS memory cannot fit, evaluate a
BLE/USB companion adapter using the same controller contract; do not silently
ship an oversized implementation or replace Zumi with XiaoZhi.

The wording “finish recording, then send” means **commit the question on finish**.
To avoid storing a complete recording on the device, chunks are uploaded during
Listening to a temporary relay session. The relay can run streaming ASR but must
not invoke the LLM or TTS until `input.commit`. Cancel deletes the uncommitted
session. The preview explains this explicitly. If absolutely no bytes may leave
until recording finishes, that is a different design requiring bounded Flash
spooling or a much shorter recording limit; it is not implemented or assumed here.

### Ownership and integration points (planned, not enabled)

- `zumi_main.c` remains the sole state/UI owner. Add Talk event dispatch rather
  than replace navigation or game state. The new `talk_model.c` has no IDF/LVGL
  dependency; firmware linkage awaits the next implementation stage.
- One audio owner arbitrates game sound and Talk. Before recording, cancel game
  audio and wait for a worker acknowledgement. Serialize init/format/read/write/
  suspend, retain BSP recovery, and never stop/free a resource still used by a worker.
- Enter Idle readiness only after Wi-Fi, verified TLS/device authentication, protocol
  negotiation and audio-owner readiness succeed; failures remain Offline/Error.
- Half-duplex: Listening reads only; Processing has no mic reader; Speaking writes
  only. Interrupt stops playback, empties queues, invalidates the turn, returns
  Idle; a second deliberate OK starts a new recording. No acoustic barge-in/AEC.
- ISR/button callbacks enqueue only. Network parsing and PCM I/O live in workers;
  only bounded UI snapshots cross to the LVGL owner. Audio has precedence over
  nonessential redraws. Redraw status at about 5-10 Hz, not per PCM frame.
- Active Talk holds a display-idle lease; release on done/cancel/error/exit.
  Idle, Offline and Error use existing 60-second screen-off and wake-card flow.
  Exit cancels workers before UI release, clears transcript buffers and closes
  the Talk connection; never mutate the saved game board or game-progress RAM.
- Every queued event includes a connection epoch and turn ID. Cancel/exit/
  disconnect invalidates them immediately. The model tests turn rejection;
  connection-epoch checking belongs to the unimplemented transport adapter.
- The current codec read/write timeout may delay worker exit by about 1 second
  plus scheduling. Use bounded join with a measured allowance (initial 1.5 s),
  fail closed on timeout, and do not start a replacement worker. A faster I/O
  timeout requires a separately tested BSP API extension; do not edit managed components.

## 4. Six states and three keys

| State | Screen | OK short | Mic / speaker |
| --- | --- | --- | --- |
| Idle | Original Zumi, ready prompt or current answer | Start a new question | Off / off |
| Listening | Original focus expression, red REC + 00-15 s | Finish and commit | On / off |
| Processing | Recognized text, waiting indication | Cancel | Off / off |
| Speaking | Answer text, playback indication | Stop; return Idle | Off / on |
| Offline | Connection unavailable, reconnect prompt | Request connection, 10 s deadline | Off / off |
| Error | Timeout/service/buffer error and retry | Clear error to Idle; no automatic recording | Off / off |

UP/DOWN scroll the text. Long OK keeps the existing 500 ms home action and must
consume the release; it never also starts recording. No touchscreen dependency.
Recording auto-commits at 15 s. Processing has a 20 s absolute deadline; partial
text never extends it. Speaking has a 60 s absolute deadline and 5 s audio-gap
limit; arriving audio does not extend the absolute deadline. A short jitter
underflow plays silence; a 5 s gap aborts. Reconnect never resends captured audio.
Authentication rejection is Error; link loss is Offline. Repeated manual retry
uses a new turn; future network connection retry is capped at 3 attempts with
1/2/4 s backoff and then waits for a button. The preview models manual connection.

Existing Zumi imagery is reused without altering the boy character or drawing
new expressions. Zumi Pocket branding remains on wake cards only on the screen;
Talk shows “Talk”, status, transcript and controls. Current English device labels
fit existing fonts. Chinese ASR/answer text requires a licensed CJK font with
verified coverage, explicit LVGL selection, bounded UTF-8 layout and hardware
review. Reserve Flash below; do not represent browser CJK as implemented firmware.

## 5. Bounded memory plan (estimates, not measurements)

Existing static allocations must not be counted twice against the linker remainder.
The 24 KiB LVGL pool and draw lists are already linked; LCD DMA, codec DMA and
tasks consume runtime memory. The old audio demo's 96 KB recording buffer is not
linked into Zumi and is not proposed for Talk.

| Additional Talk item | Budget bytes | Bound |
| --- | ---: | --- |
| Uplink slots | 3,200 | 5 x 640, 100 ms maximum queued PCM |
| Downlink jitter slots | 7,680 | 12 x 640, 240 ms; start after 3 frames / 60 ms |
| Capture / playback scratch | 1,280 | Two 640-byte blocks |
| Controller + transcript snapshots | 2,588 | Measured `sizeof(t_model)`; includes ASR 1,024 and answer 1,536 |
| JSON RX / TX | 2,560 | 2,048 / 512 bytes |
| Bounded parser arena | 4,096 | Explicitly capped; reject oversize before parse |
| Queue descriptors / events | 1,024 | Fixed counts, no unbounded message strings |
| Additional worker stacks | 10,240 | Initial 4 KiB + 6 KiB; check stack high-water marks |
| **Application subtotal** | **32,668 (~31.90 KiB)** | Ring/parser/worker code is planned, only controller is currently implemented |
| Wi-Fi / lwIP increment | 45-65 KiB | Planning allowance; connection settings and task lifetimes matter |
| TLS / WSS increment | 35-60 KiB | Includes handshake working memory; depends on certificate chain/client configuration |
| **Additional peak planning range** | **~112-157 KiB** | Plus allocator/driver uncertainty; not a fit guarantee |

The high end may exceed the real remaining memory after existing display/audio
startup. RAM, not Flash, is the likely go/no-go constraint. Allocate fixed audio
arenas before handshake; never grow queues to hide slow links. If resources are
insufficient, show Error and keep the offline game usable.

Provisional acceptance targets, to be calibrated from measurements: >=32 KiB
free internal 8-bit heap and >=16 KiB largest block in steady Talk; >=40 KiB
largest block before TLS handshake; >=1 KiB minimum stack headroom on each new
worker. These are engineering gates, not proven ESP-IDF requirements. Do not
reduce TLS validation or use insecure transport just to pass a memory target.
If the target fails, reduce network buffer/task counts using measured evidence,
then consider the companion path. Do not enlarge the LVGL pool by default.

Flash planning: reserve 0.6-1.2 MiB for network/TLS/protocol/UI additions and
0.4-1.2 MiB for a licensed CJK font if Chinese display is selected (estimates,
not compiled sizes). With the current app this suggests 3.62-5.02 MiB remaining;
exact sizes require a new linked build. No new full-screen images, video, wakeword
model, full conversation log or recording file partition. No OTA migration in V1.

### Runtime measurement recipe (not executed)

At boot, home, game with sound, Wi-Fi start, post-association, before/after TLS,
Listening, Processing, Speaking, cancellation and cleanup collect:

```c
const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
heap_caps_get_free_size(caps);
heap_caps_get_largest_free_block(caps);
heap_caps_get_minimum_free_size(caps);
// Also capture MALLOC_CAP_DMA separately, each task's stack watermark,
// LVGL pool usage, queue high-water marks, dropped frames and audio feed gaps.
```

Do not add DMA and internal-heap totals: the capability pools overlap. Poll memory
periodically during TLS/streaming as well as at phase boundaries; a before/after
snapshot can miss peaks. In the ESP-IDF FreeRTOS port, record stack watermark
units from the installed API documentation. Logs contain counters and error codes
only, never credentials, transcript, PCM or user identifiers. Repeat 100 start/
stop/disconnect cycles and 30 minutes of Talk/game alternation; look for monotonic
heap/largest-block loss, task leaks and audio starvation.

## 6. Replaceable device/relay contract (proposal, no live endpoint)

Transport adapter operations: `connect`, `begin(turn, capability)`,
`send_audio(frame)`, `commit(last_sequence)`, `cancel(turn)`, `close`.
Provider adapters on the relay expose streaming ASR, one-shot question/reply
stream and TTS stream. Provider API keys stay server-side, injected outside Git.
Use verified TLS, hostname validation and limited-lifetime device credentials
provisioned at runtime; no provider key or private endpoint in the image. Secure
provisioning, device token refresh and clock/certificate bootstrap remain work
items; “no API key in firmware” does not mean an unauthenticated public relay.

Proposed control message examples (UTF-8 JSON, <=2,048 bytes RX / 512 bytes TX):

```json
{"v":1,"type":"hello","capabilities":["qa"],"audio":{"codec":"pcm_s16le","rate":16000,"channels":1,"frame_ms":20}}
{"v":1,"type":"input.begin","turn":42,"capability":"qa","memory":"none"}
{"v":1,"type":"input.commit","turn":42,"last_seq":749}
{"v":1,"type":"asr.final","turn":42,"text":"What is solar power?"}
{"v":1,"type":"answer.snapshot","turn":42,"seq":0,"text":"Energy from sunlight."}
{"v":1,"type":"output.end","turn":42,"last_seq":99}
{"v":1,"type":"cancel","turn":42}
```

Each binary frame: version u8=1, type u8 (1=input, 2=output), flags u16LE=0,
turn u32LE, sequence u32LE, payload length u16LE, followed by 1-640 even PCM bytes.
Header length 14; no raw C-struct casts. Fixed per-direction sequence begins at 0.
Validate complete header, exact length, version, flags, negotiated format and
current connection epoch/turn before queueing. A bounded fragmented-WebSocket
assembler accepts at most 654 bytes binary or 2,048 bytes text; reject oversized,
invalid UTF-8, unsupported formats and malformed JSON without allocations based
on attacker lengths. `answer.snapshot` replaces a bounded rolling text window;
no accumulated history. Out-of-order/gap frames fail the turn; duplicates can be
ignored only when already accepted in the same turn.

On recording end: stop capture, drain uplink at most 500 ms, require contiguous
sequences through `last_seq`, then commit once. Queue full/network stall -> abort
with Error, rather than silently submit missing words. Discard residual chunks
on cancellation. On downlink, advertise available frame credits; the relay never
exceeds credits. Overflow is an error, never a queue expansion. `output.end`
means no more input from the server; the adapter emits model DONE only after the
local player drains. If final audio arrives out of sequence, fail rather than
show a false completed response. The review model tests control states only;
these wire-format, queue and drain contracts still need implementation tests.

One turn is the default conversation scope. Clear client text/audio on next turn,
exit, cancel or disconnect; no NVS/Flash history, wakeword engine or always-on
mic. Relay memory is ephemeral, with session cleanup on commit/cancel/disconnect
and bounded orphan TTL (proposed 60 s). Provider retention settings require
separate verification before a real service is selected; no unverified privacy
promise is made. Future capabilities `sound_guess`, `story` and `garden_control`
are reserved only. V1 accepts `qa`; reject unsupported capabilities and do not
execute any remote tool/Garden command. Garden control will need its own explicit
local confirmation and command allowlist.

## 7. Next integration gates

1. Approve interaction semantics, Chinese-display scope and relay deployment choice.
2. Instrument real heap/stack/DMA measurements; add audio ownership acknowledgement
   and bounded transport with a local fake relay, without paid-provider access.
3. Implement STA/provisioning lifecycle and the normalized PCM service adapters;
   measure handshake and sustained streaming before deciding any compression.
4. Test malformed frames, oversize JSON, authentication rejection, server silence,
   slow links, dropped sequence, queue overflow, output-end-before-drain and late
   responses after cancellation. Verify no capture/uplink outside Listening.
5. Confirm 4 x 4 game logic, sounds, wake-card resume and 60-second screen-off
   regressions; preserve all Zumi assets and BSP pin/partition definitions.
6. Device flashing remains subject to a separate explicit approval; paid-service
   activation also requires confirmation. No approval is inferred from this review.
