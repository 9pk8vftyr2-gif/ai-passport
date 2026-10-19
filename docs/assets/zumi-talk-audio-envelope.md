[简体中文](zumi-talk-audio-envelope.zh_CN.md)

# Talk: audio-driven mouth and continuous playback

2026-10-19. Keeps the original character, centered microphone label, games,
three-second wake card and game resume. No device flash or paid service.

## Behavior and architecture

Speaking uses the amplitude of rendered audio instead of a 140 ms animation timer.
The browser audio thread reports mean absolute PCM amplitude every 20 ms; the
shared C controller opens above 450 and closes below 220 (PCM16 scale). This
hysteresis reduces flicker near the threshold. Silence, prebuffering and rebuffering
report zero. Stale turn reports are ignored; missing reports close the mouth after
160 ms. Completion/cancellation returns to the original idle expression. These
are two original mouth frames, not phoneme-specific lip sync or a redesigned Zumi.

An AudioWorklet replaces per-packet AudioBufferSource scheduling on the page
thread. It consumes a bounded queue continuously, linearly resamples 16 kHz PCM
to the output context rate, and returns credit only for consumed frames. Official
desktop playback still has a 36-frame ceiling (720 ms, 46,080 bytes float PCM),
with 15 frames (300 ms) of startup/rebuffer content; mock playback uses 12 maximum,
6 before starting. Short replies drain when the end marker arrives without waiting
for a full buffer. A three-millisecond attack applies only on start/rebuffer,
not at packet boundaries. Cancel disconnects and stops the processor; stale async
module loads cannot revive a cancelled turn. No entire utterance is retained.

This improves tolerance of page stalls without unlimited buffering. Long network
outages can still exhaust the queue. Reported underruns are now counted by the
audio renderer, rather than inferred from page scheduling time. A zero count does
not prove the cloud TTS itself has no gaps or that speakers sound natural.

The device adapter derives the envelope from successful BSP PCM writes and
expires it after 80 ms without a write. The existing 12-frame device budget stays
unchanged; no new image buffers or audio task are added. Codec DMA latency and
physical mouth/speaker alignment require hardware measurement. Desktop audio
thread results must not be treated as ESP32 playback performance.

## Validation

See the [sanitized receipt](talk-v1/audio-envelope.json) for exact build identity,
counts and limitations. No recordings, tokens or account identifiers are included.

- Build: PASS, ESP-IDF 5.5.3; app 2,757,312 bytes, 5,565,760 bytes free in factory.
- Host Tests: PASS, complete gate, mouth thresholds/silence/stale reports, BSP
  write/cancel/expiry, and playback at 16/44.1/48 kHz including partial tails,
  exact credits, overflow, drain, stop and rebuffer.
- Browser Demo: PASS, tone–800 ms silence–tone visibly gives open–closed–open;
  closed remains stable across a 350 ms observation. Microphone permissions,
  cancellation, local game/wake/resume and reconnect tests pass.
- Controlled playback: 240/240 frames; 250 ms delivery stalls plus 400 ms UI
  stall; zero renderer underruns, queue no larger than 36.
- Official XiaoZhi: public synthetic greeting, three completed turns and one
  interrupted turn followed by recovery; exact receipt records final run counts.
- Device Tests: NOT RUN.
- Unverified: human microphone accuracy, subjective listening quality, output
  device latency, physical display/speaker synchronization, runtime heap/power,
  long network stalls and full login/reboot acceptance.

Firmware bundle:
`build/firmware/843964406016c348fbd81cf11c6eab67b6accd76129eefb92fd376cbe1f2162d/`.
Full image SHA-256: `843964406016c348fbd81cf11c6eab67b6accd76129eefb92fd376cbe1f2162d`.
Matching ELF SHA-256: `399bac38c77e524571cb917151c7c6893e1d9bf37fac02bf2f17ac90ad81c2fb`.
Embedded source version is `ceb35d5-dirty`; the archive hashes identify the tested
artifacts, not a subsequent rebuild. Archive verification passed. Nothing was flashed.

The installed desktop snapshot is refreshed. Reload
`http://127.0.0.1:8876/preview/?talk=xiaozhi`, enter Talk, then use OK to record,
OK to send, and OK during playback to interrupt. Website spelling and persona
remain as previously configured; the test never automatically uses a real microphone.
