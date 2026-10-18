[简体中文](zumi-talk-integration.zh_CN.md)

# Zumi Talk: local integration and service identity

Desktop official-service extension: [XiaoZhi trial and setup](zumi-talk-xiaozhi.md).
The firmware and default preview described below still use the local adapter.

2026-10-18. This increment follows the approved Talk design and supersedes the
review-only status in [the original audit](zumi-talk-v1.md). The original audit
and its hashes remain historical evidence; they are checked against their pinned
Git revision, not against the now-modified application.

## Delivered behavior

The existing Home now opens the shared six-state Talk controller. Original Zumi
art, Energy Link's six 4 x 4 boards, game tones, eight promotional wake cards and
three-second game resume remain in place. No website repository is modified.

- Enter Talk: automatically connect with the microphone off, then show Idle.
- OK starts capture, the next OK commits; OK retries after connection failure. UP/DOWN scroll the introduction or transcript.
- Processing and Speaking can be interrupted with OK. Hold OK returns home.
- Recording has a 15-second limit, processing 20 seconds, playback 60 seconds,
  missing audio 5 seconds, and connection 10 seconds. Error retry never resends audio.
- Active Talk holds the screen awake. Idle/Offline/Error still turn off after
  60 seconds without a key; screen-off and leaving Talk stop the session.
- A wake card still returns to the preserved game after three seconds, or Home
  when waking outside the game. The waking key does not start a recording.
- No continuous listening, conversation persistence or automatic next recording.

## What is actually connected

The main preview uses a real local WebSocket connection and Web Audio playback.
Its capture source is **synthetic PCM**, not the browser microphone. ASR and
answers are fixed fixtures; this does not demonstrate speech recognition or AI
reasoning. Select **Meet Zumi** or **Solar power** in the preview's test controls.
The speech fixture uses a local system voice, not an approved final Zumi voice.

Firmware links a Wi-Fi STA/WebSocket adapter and the original ES8311/I2S BSP.
It compiles but has not run on a board. Its local adapter reads existing NVS
`zumi_talk` fields `ssid`, `password`, private IPv4 `host` and optional uint16
`port` (8881). There is **no provisioning UI or credential writer in this
increment**. Missing configuration remains Offline. No NVS erase, reset, flash,
or credential logging has occurred on a physical device.

The local endpoint intentionally supports only private LAN IPv4 and plain WS.
It has no production authentication or TLS and is not a public service. The
mock binds to loopback by default. The optional LAN bind is for a later approved
board test on a trusted test network; browser localhost is not a device address.

## Try it locally

From the repository root, in separate terminals:

```sh
python3 -m http.server 8876 --bind 127.0.0.1
python3 -m venv build/talk-venv
build/talk-venv/bin/pip install -r tools/talk_mock_requirements.txt
python3 tools/build_talk_mock_audio.py
build/talk-venv/bin/python tools/talk_mock_server.py
```

The audio generator needs macOS `say` with Daniel at 180 words/minute with brief phrase pauses and `ffmpeg`; alternatively
supply matching mono 16 kHz signed PCM16 WAV files at `build/talk-mock/intro.wav`
and `solar.wav`, using the text in `tools/talk_mock_fixtures.json`. Generated
system-voice audio stays in ignored `build/`, is not redistributed, and is not
embedded in firmware. Open <http://127.0.0.1:8876/preview/>. The former
`/preview/talk/` page is retained only as the original visual design reference.

Mock failure modes: `--fault error`, `disconnect`, `timeout`, `bad-frame`.
An alternate local port can be selected with `--port` and preview `?talk_port=`.
The service keeps only bounded connection/turn counters, never writes input
recordings, and expires an idle connection after 60 seconds.

## Zumi identity and the XiaoZhi core

**Zumi is the user-facing character; XiaoZhi is the selected desktop voice
backend.** The firmware and default preview still use the local mock. The identity configuration
at [config/zumi-persona.json](../../config/zumi-persona.json) is provider-neutral,
contains no credentials, and has not been installed in a hosted agent.

- Zumi is the ZCO green living companion. Talk omits the website badge and does not emphasize gender. He is warm, curious and concise,
  explains green ideas through everyday examples, and can answer ordinary questions.
- Zumi Pocket is the portable interactive product; `zco.eco` is the ZCO website.
  The website supplies character/community context, games and tools. Pocket has
  no website account synchronization or Garden control in this version.
- Reviewed knowledge is a dated snapshot, separate from identity. A URL in a
  prompt does not make the model know or browse the website. Current facts were
  checked read-only against local website source; live website retrieval failed.
  Unknown or newer website claims must remain unknown until refreshed.
- Explain the actual underlying service honestly when asked. No default long-term
  conversation memory. Hosted retention/memory settings need separate verification.
- Accept with questions about identity, ZCO, the website relationship,
  actual service, account access, Garden control and whether scores measure carbon.
  The fixture validates presentation, not an LLM's adherence to this role.

[The XiaoZhi WebSocket specification](https://github.com/FoloToy/folo-ai-passport-xiaozhi/blob/72da544d1b51678f5d88967adc299b3aec956946/docs/websocket.md)
uses device/client identity and authorization headers, a hello/session exchange,
Opus frames, manual listen start/stop, abort, STT and TTS messages. The local PCM
protocol is **not compatible by changing a URL**. Direct connection needs a
separate adapter, Opus encoding/decoding, negotiated sample-rate handling, TLS,
device activation and runtime memory validation. Do not copy the complete port
or replace Zumi's UI/partitions. Keep this application on ESP-IDF 5.5.3; the
referenced port has different SDK requirements. XiaoZhi console configuration
availability, account binding, custom voice/role options, knowledge integration,
retention and service terms must be checked in the selected account before use.
The desktop trial uses the user-confirmed free official service; no paid service
was selected. A controlled relay remains a fallback
if Opus plus TLS cannot fit the measured C3 heap.

## Ownership, protocol and memory

`core.c` owns navigation/Talk state; `view.c` draws the shared UI. Device worker
queues never touch LVGL. `talk_device.c` owns STA, events and the socket;
`audio.c` is the sole owner of BSP audio I/O for Talk and game tones. Shutdown
invalidates the turn/generation before draining queues. A stop acknowledgment
means the audio worker returned from its current BSP call and applied the stop;
commit waits for this acknowledgment before sending the final frame number.
An in-flight partial capture block is discarded on stop. BSP calls may take up
to their existing one-second I/O timeout; actual interruption latency is unmeasured.
Playback completion waits for the application queue plus a 100 ms DMA tail guard,
which requires hardware verification.

The mock protocol sends `hello/ready`, `input.begin`, ordered audio chunks,
`input.commit`, `asr.final`, `answer.snapshot`, output chunks, and `output.end`.
Capture streams before commit; commit means the user has finished and requests
an answer. Each frame is 640 bytes/20 ms PCM16 at 16 kHz mono, plus a 14-byte
little-endian version/type/flags/turn/sequence/length header. Credits cap output
at 12 outstanding application frames. Credit-based pacing and a 120 ms browser
prebuffer avoid cumulative gaps from per-frame server sleeps. Audio activity
notifications are coalesced to 200 ms on the device to avoid UI-queue bursts. Stale turns are dropped; bad sequence,
format, sizes or overflowing queues stop the session. JSON is capped at 2048
bytes, nesting 4 and 48 structural/string tokens before cJSON parsing. The
current fixture/display contract is English ASCII; Chinese screen fonts remain
future work. TCP-split payloads are assembled with a one-second deadline;
fragmented WebSocket messages are rejected. Cancellation during a partial
payload closes the connection. All networking calls run outside key callbacks.

| Allocation | Budget / evidence |
| --- | --- |
| Application state | 2700 bytes each; two static state snapshots; Talk model 2588 bytes |
| Existing draw lists / LVGL pool | 2 x 6404 bytes / 24576 bytes, included in linked static memory |
| Existing LCD buffer | 19200 bytes allocated at runtime, not another full screen buffer |
| PCM queues | 5 x 648 + 12 x 648 = 11016 bytes, plus FreeRTOS queue metadata |
| UI notification queue | 4 x 1548 = 6192 bytes plus metadata |
| Command queue | 8 x 12 = 96 bytes plus metadata |
| Worker / main stack | 6144 network, existing 4096 audio, 6144 main (raised by 2048) |
| Static network scratch | 2 x 2049 JSON buffers, 654 wire bytes, 2 x 1548 notifications; included in linked static memory |
| Wi-Fi buffers | Static RX 6, dynamic RX 8, dynamic TX 8; bounded to reduce no-PSRAM peaks |
| Remaining runtime costs | Wi-Fi/LwIP/task stacks, I2S DMA and driver state, cJSON, socket buffers, allocation overhead; need board measurements |
| TLS / Opus | Not implemented or included in measured runtime figures |

Linked RAM headroom is not free heap. Sample internal free, largest and minimum
heap at before-connect/connected/listening/speaking/completed. Record game-only,
Wi-Fi-up, repeated turns, disconnect and post-exit values before selecting the
real backend. Buffers and task queues are reused across turns; they are not
allocated per recording and do not hold a whole utterance.

## Validation and hardware acceptance

See [integration.json](talk-v1/integration.json) for exact build hashes,
source hashes, partition sizes, map totals and test status. This file is updated
only from completed checks. Baseline historical audit stays separate.

| Field | Scope |
| --- | --- |
| Build | Official full ESP-IDF 5.5.3 gate, merged image and matching ELF verification |
| Host Tests | Core/wire/state/audio-owner tests; queue and cancel fault injection; model/game native-WASM parity; seven actual local WebSocket tests |
| Browser Demo | Main preview, spoken fixture, interruption, background close/reconnect, error/disconnect/timeout/bad frame recovery, game and all wake cards |
| Device Tests | NOT RUN; no serial access, flashing, erase or reset |
| Unverified | Real mic/speaker/Wi-Fi, free/largest/minimum heap, cancellation and DMA drain timing, power/endurance, provisioning, device TLS/auth/Opus, Chinese display, production voice and model persona adherence |

After hardware arrival and separate test authorization: validate safe key/LED
recording indication; record/play/cancel at every phase; check speaker/game audio
ownership; wrong password/unavailable AP/dropout/reconnect; heap and stack peaks
including 100 repeated turns; one-hour offline game/wake regression; measure
current with screen off, Wi-Fi idle, capture and playback. Real service acceptance
must cover device binding, role/knowledge questions, disabled automatic listening,
retention settings and key handling without adding secrets to the repository.

## Results for this increment

Build **PASS**; Host Tests **PASS**; Browser Demo **PASS**; Device Tests **NOT RUN**.

Application 2,742,464 bytes; factory remaining 5,580,608 bytes; linked DIRAM headroom 138,722 bytes, **not runtime free heap**. Full image SHA-256: `c6b3dc0d19adf0c809912723d7474e8d2ba0fc6c772d1acbbb23b28a61705117`; matching ELF: `c5f54f2ca1e4065172a90aba90dfc4987367c5b8be08c72292366f16eeb09da9`.

Browser evidence covers a complete playback and two cancellations: 490 received frames, 466 played, maximum queue 12, underruns 0. The 24-frame difference consists of queued frames discarded on cancellation; the complete turn separately asserts that every received frame plays.
