[简体中文](zumi-talk-recognition.zh_CN.md)

# Talk capture, pronunciation and recovery

2026-10-18. Desktop changes only; no physical-device operations. The earlier
[persona acceptance](zumi-talk-persona.md) remains a historical snapshot.

## Changes

- The browser displays microphone preparation, ready, quiet/loud input and off
  status. Recording still requires OK, never page entry. Permission denial and
  late grants stop cleanly. Level hints are approximate, not calibrated meters.
- Capture requests the existing echo cancellation, noise suppression and gain
  control, prefers native 16 kHz AudioContext resampling, and applies 70 Hz
  high-pass / 7 kHz low-pass filters. The bounded worklet retains fallback
  resampling. These are signal-path changes, not a measured human-ASR gain.
- OK-to-send flushes the final partial PCM frame before commit. At most one
  20 ms frame is padded. Flush is bounded to 500 ms and is cancelled on exit or
  interruption; no pending callback may submit an abandoned turn.
- Official Zumi role instructions now require the brand to be emitted as
  `Z C O` and spoken as separate English letters. Chinese speech prefers Zumi's
  established Chinese name. In clear green-living contexts, likely homophones
  of the carbon-reduction term receive cautious clarification and explanation.
  Ambiguous terms must not be blindly rewritten.
- The available console controls did not expose an ASR hotword list. Role
  instructions affect understanding/TTS, not the ASR decoder. Raw transcripts
  remain visible and unchanged; no displayed-text replacement masks ASR errors.
- Successful turns reuse the current cloud session, allowing follow-up references.
  Cancellation sends abort and closes the session; errors, leaving Talk,
  disconnects and idle expiry also clear it. A new session after interruption
  isolates old audio; prior conversational context is not guaranteed then.
  Official long-term Memory remains off. Platform retention is a separate matter.

## Observed tests

The [receipt](talk-v1/recognition-recovery.json) separates raw ASR, understanding,
audio transport and user acceptance. All automated voice input was synthetic;
the agent did not activate the user's real microphone.

| Check | Observation |
| --- | --- |
| Brand output | Responses emitted `Z C O`; official role save/readback matched |
| Raw Zumi recognition | Chinese name recognized in test turns |
| Raw ZCO recognition | Misspelled on the first term test; correct on the recovery question |
| Raw carbon-reduction recognition | Still misrecognized as a homophone; accuracy improvement is **not established** |
| Current-session follow-up | Correctly recalled that the previous turn discussed travelling by bicycle |
| Cloud interruption | Interrupted a long-story answer, then completed a new website-games answer |
| Browser sequence | Two full turns, one interruption, one recovered full turn; four microphone requests, all tracks stopped |
| Audio bounds | 495 frames received, 486 played, nine intentionally discarded on interruption; max queue 12, underruns 0 |
| Microphone lifecycle | Permission denial, late grant cleanup, blur cancellation and final-frame-before-commit passed with fake browser media |
| Worklet | 16/44.1/48 kHz bounded framing, partial tail preservation and finish ordering passed |

An additional contextual homophone rule was applied after the initial term test;
it has not been used to re-label that failed raw-ASR result as successful.
New tests are `tests/test_talk_worklet.cjs`, the extended
`tests/browser_talk_microphone.cjs`, `tests/test_xiaozhi_relay.py` and the opt-in
`tests/browser_xiaozhi_live.cjs`. No cloud calls run in the default validation gate.

## Validation

- Build: **PASS**, complete repository gate with ESP-IDF 5.5.3; archive re-verification passed.
- Host Tests: **PASS**, including the separate relay and capture-worklet checks.
- Browser Demo: **PASS**, local integration/game-wake regression, microphone lifecycle and opt-in cloud sequence.
- Device Tests: **NOT RUN**; no flashing or device-storage operations.
- Unverified: human ASR accuracy/listening preference, physical audio, runtime heap and endurance.

The desktop changes do not add an official-cloud transport to the device firmware.
The checked application is 2,742,464 bytes, with 5,580,608 bytes free in its factory
partition. Exact merged-image/ELF hashes and archive path are in the receipt.
This build was made from the pre-commit working tree (`2cd594d-dirty`); artifacts
remain local and are not published.

## Human acceptance

Refresh <http://127.0.0.1:8876/preview/?talk=xiaozhi>, enter Talk, press OK and
wait for the ready hint before speaking. Try the Chinese name, separately spoken
Z/C/O letters and the carbon-reduction term, then compare the untouched transcript
with the intended sentence. Repeat at ordinary speaking distance and volume,
then interrupt a response and ask again. These are checks, not a request to store
or upload recordings. Ordinary runtime records no audio or transcripts to disk.

Device Tests: **NOT RUN**. Runtime heap, physical mic/speaker quality and battery
remain unverified. Human pronunciation preference and actual microphone accuracy
also remain unverified until the user performs the trial. A cloud configuration
change does not establish improved acoustic recognition on the future device.
