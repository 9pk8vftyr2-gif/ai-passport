[简体中文](zumi-talk-human-check.zh_CN.md)

# Talk real-microphone check

2026-10-19. Real microphone input in the installed desktop preview, using the
previously authorized free official XiaoZhi service. Based on `6659bbb` plus
visible browser diagnostics. No device was flashed and no paid service enabled.
No raw recordings, private conversations or identifiers are included in this report.

## Findings

| Item | Result | Evidence and scope |
| --- | --- | --- |
| Zumi, spoken as its Chinese name Zu Mi | NOT STABLE | In two controlled attempts, one was correctly recognized as Zu Mi and the other as Zong Yi. The user confirmed saying Zu Mi in the failed attempt. This is not an accuracy estimate from a representative sample. |
| ZCO, spoken as separate letters | PASS in observed sample | The combined keyword prompt retained ZCO in the original service transcript. |
| Mandarin jian tan (carbon reduction) | PASS in observed samples | Correct in the combined prompt and the post-interruption short question. |
| Speech continuity | PARTIAL | Initially the user reported stuttering with roughly synchronized mouth movement. Later the user confirmed continuous playback; complete replies drained 354/354, 987/987 and 282/282 frames, with zero renderer underruns. The initial fault was not captured with per-turn diagnostics and its cause is unresolved. |
| Interruption and recovery | PASS in this trial | The user confirmed the old reply stopped, a new question worked and the recovered reply was continuous. Its queue drained 282/282 frames. No numerical interruption-latency measurement was made in this human trial. |
| Mouth rhythm and silence | PASS by user observation in final trial | User confirmed silence closes the mouth and new speech resumes normally. Not a frame-by-frame audiovisual timing measurement. |
| English spelling “Zumi” in ASR | NOT TESTED | Controlled microphone prompts used Zu Mi. |

The browser reported output at 44.1 kHz, processing latency about 6 ms and output
latency up to 186 ms during these samples. These are browser estimates, not a
physical speaker/visual synchronization measurement. Device type was not confirmed.
ZCO's response text uses separate letters; independent pronunciation scoring was not performed.

The foreground policy is unchanged: leaving the window cancels Talk and returning
reconnects. Keep the preview foreground while listening, then return to chat.
This is a known cancellation path, not proof that it caused the initial stutter.

## Test support added

The browser now shows a per-turn count of received/played frames, renderer
underruns, largest arrival gap, maximum queue, reported output latency and a stop
reason. It keeps only the latest turn's counters in page memory, without recording
audio or persisting transcripts. Completion records survive focus/reconnect until
a new recording starts. Rendering of the test panel is throttled to five updates
per second. This instrumentation does not change audio buffering, recognition,
provider voice or firmware behavior.

The simulated device currently has an ASCII-only firmware font. `Answer shown below`
is a placeholder, not the actual answer. A link below the device, a Chinese transcript link,
jumps to the webpage's existing original transcript and Chinese answer. There is
no transcript correction or substitution to hide recognition errors.

## Validation and next acceptance

The [sanitized receipt](talk-v1/human-microphone.json) records validation and build
identity. Human acceptance is **PARTIAL**, regardless of automated checks passing.
The complete gate and browser regressions validate the diagnostic addition, not
ASR reliability. Device Tests are **NOT RUN**.

Remaining: repeat Zu Mi across natural phrasing and input distances, establish a
representative keyword sample, reproduce intermittent stuttering with diagnostics,
measure audiovisual timing if still perceptible, and eventually test physical
microphone/speaker/display, heap and power after separate hardware authorization.
Do not silently map arbitrary misrecognized phrases to Zumi. A provider-side
recognition/hotword change needs a supported API or console control and its own test.
