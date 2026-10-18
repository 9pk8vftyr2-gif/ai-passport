[简体中文](zumi-talk-xiaozhi.zh_CN.md)

# Zumi Talk: official XiaoZhi desktop trial

2026-10-18. This extends the [local integration](zumi-talk-integration.md).
The user bound a distinct software test client in the official console and
confirmed free service availability. A verified TLS WebSocket handshake and
one complete synthetic-speech round trip passed. This is a **desktop** result;
the ESP32-C3 firmware still uses the local PCM adapter, not direct cloud Opus/TLS.

## Try the conversation

Open <http://127.0.0.1:8876/preview/?talk=xiaozhi>. Select Talk: connection is
automatic with the microphone off. Press OK, allow microphone access, speak,
then press OK to end recording and await the answer. Press OK during a response
to interrupt; hold OK to return home. Each recording is limited to 15 seconds.
Leaving or hiding the page stops capture/playback and closes the local session.
The browser streams audio while recording; it does not upload a saved utterance
afterwards. In manual mode, the stop message requests the response.

Use [the role setup page](../../preview/talk/setup.html) to copy the prepared Zumi
instructions into the bound device's official agent settings. Choose an available
free male voice, disable long-term memory, save, exit Talk and re-enter. These
console settings **were saved and read back on 2026-10-18**: custom Zumi role,
Sunshine Youth male voice, standard reply speed/pitch, and Memory off. See the
[role acceptance report](zumi-talk-persona.md). No role or voice is injected through an undocumented protocol.
Knowing the website URL does not grant live browsing or account access.

The usual <http://127.0.0.1:8876/preview/> remains the local synthetic mock. The
official route is explicit and uses a separate loopback bridge on port 8882.
The original character, offline game and wake behavior are unchanged.

## Start the desktop services

Use Python 3.9+ and the existing pinned WebSocket dependency:

```sh
python3 -m venv build/talk-venv
build/talk-venv/bin/pip install -r tools/talk_mock_requirements.txt
python3 -m http.server 8876 --bind 127.0.0.1
```

In another terminal, install libopus using the host package manager or set
`ZUMI_OPUS_LIBRARY` to an existing trusted local libopus shared library. The
project does not redistribute it. Then:

```sh
python3 tools/xiaozhi_setup.py request-binding
# Bind the displayed code in the official console, then refresh metadata:
python3 tools/xiaozhi_setup.py request-binding
build/talk-venv/bin/python tools/talk_xiaozhi_relay.py --probe
build/talk-venv/bin/python tools/talk_xiaozhi_relay.py
```

The same private desktop identity is reused. Do not request a new code if the
client is already bound; `status` reads local state without a network request.
Metadata retrieval ignores firmware updates. Probe performs hello only, with
no audio/inference. Actual recording requires the browser action above.

Issued credentials and software identity live in the user's private application
support directory, **outside both Git and the HTTP document root**, with 0700
directory / 0600 file permissions. Never paste tokens into chat, URLs, source or
build logs. The relay requires a matching local browser Origin and allows one
client at a time. It binds only to 127.0.0.1, not the LAN.

## Protocol and bounds

Browser AudioWorklet converts mono microphone input to 16 kHz PCM16, emitting
320 samples / 20 ms. A separate capture AudioContext is closed at stop; a late
permission grant immediately stops its tracks. No automatic microphone request
occurs on opening the page or entering Talk.

The bridge combines three PCM frames into one 60 ms Opus packet. It validates
turn/sequence/length and pads at most one final packet. Verified TLS and hostname
checks stay enabled. It implements WebSocket protocol version 1, hello, manual
listen start/stop, abort, STT, TTS sentence text and raw Opus audio, following the
[upstream protocol](https://github.com/FoloToy/folo-ai-passport-xiaozhi/blob/72da544d1b51678f5d88967adc299b3aec956946/docs/websocket.md).
No MCP, camera, desktop tools, remote reboot or firmware commands are enabled.

The server negotiated 24 kHz mono Opus; libopus decodes directly to 16 kHz for the
existing playback path. At most 12 PCM frames (240 ms) are scheduled in-browser;
initial prebuffer is 120 ms. Playback credits pace delivery. The decoder holds
at most 120 ms PCM, the uplink packet accumulator at most 60 ms, and WebSocket
receive queues are capped at four messages. Cloud JSON is capped at 64 KiB.
These are desktop bounds, not a validated C3 memory budget.

Successful turns now reuse the current live Talk session for contextual follow-ups.
Cancellation or errors close it; recovery creates a fresh session to isolate old
audio. Exit, disconnect or idle timeout also ends the session. No durable local
conversation history is stored. See the [recognition and recovery report](zumi-talk-recognition.md).
Text is bounded, UTF-8 clipped safely, and displayed through `textContent`.
Chinese replies appear in the browser transcript; the shared hardware screen
shows a short ASCII pointer because device Chinese glyph support is unfinished.

No recordings/transcripts are written by the runtime, and no local long-term
memory is implemented. Closing sessions does not prove deletion or disable
official-platform retention; check those controls separately in the account.

## Validation

| Category | Result |
| --- | --- |
| Build | PASS for the existing IDF 5.5.3 local-adapter firmware; no firmware change for this desktop bridge |
| Host Tests | PASS: binding/secret handling, real libopus round trip, offline relay complete turn, credit/sequence bounds, cancellation, fresh session, cloud error and UTF-8 clipping |
| Browser Demo | PASS: official secure hello, public synthetic greeting recognized, reply text received, 123/123 audio frames played, max queue 12, zero observed underruns; no actual microphone used by automation |
| Capture lifecycle | PASS with fake browser media: no request before OK, permission denial, cancellation on blur, late permission grant cleanup |
| Device Tests | NOT RUN; no flash, erase, reset or serial operation |
| Unverified | Actual human microphone/acoustics, subjective voice quality, hosted retention, sustained network recovery, C3 Opus/TLS/heap, Chinese device fonts |

The official test used the public synthetic sentence "Hello. Please say hello
in one short sentence." Only non-sensitive counts are retained in
[the receipt](talk-v1/xiaozhi-desktop.json). The live browser test is opt-in via
`ZUMI_TEST_OFFICIAL=1`; it must never run automatically in CI or the firmware gate.
Offline protocol tests run with:

```sh
python3 tests/test_xiaozhi_setup.py
build/talk-venv/bin/python tests/test_xiaozhi_relay.py
build/talk-venv/bin/python tests/test_talk_opus.py
```

Before hardware acceptance: complete console role/voice/memory checks; measure
runtime heap and largest block with Wi-Fi/audio/display active; implement and
budget the device cloud adapter or a trusted relay; confirm Chinese font coverage;
then separately authorize physical tests. Desktop success is not flash approval.
