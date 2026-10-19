[简体中文](zumi-talk-desktop-reliability.zh_CN.md)

# Desktop Talk startup, playback and speaking animation

2026-10-19. No device was flashed. Official speech is still a desktop adapter;
the device firmware retains the local PCM adapter.

> Historical acceptance record. The fixed timer animation and source-node playback
> described below are superseded by [audio-driven mouth and playback](zumi-talk-audio-envelope.md).

## Installed services

`tools/talk_autostart.py` installs three user LaunchAgents with RunAtLoad,
KeepAlive and a ten-second restart throttle: loopback preview 8876, local mock
8881 and official bridge 8882. Login starts listeners, not recording or inference.
Closing a Codex task does not own their process lifetime. The user must be logged
in; system sleep, missing networking or an expired account can still interrupt Talk.

The installed runtime is a selected snapshot under
`~/Library/Application Support/Zumi Pocket/desktop-runtime`, because macOS denied
the background Python process access to the checkout's Documents-based virtualenv.
It does not require broad access to Documents. Credentials remain in the parent
application-support directory, outside the HTTP root; no credentials enter plists.
The snapshot needs refreshing after code/asset changes. The existing Python and
libopus paths must remain available. No new paid service or system package was added.

```sh
# Run from the checkout with the existing Talk environment and local libopus path.
ZUMI_OPUS_LIBRARY=/absolute/path/to/libopus.dylib build/talk-venv/bin/python tools/talk_autostart.py install
build/talk-venv/bin/python tools/talk_autostart.py status
build/talk-venv/bin/python tools/talk_autostart.py remove
```

`remove` unloads and removes only these three LaunchAgents. It keeps the runtime
snapshot and account configuration. Logs are discarded, so there is no unbounded
service log containing speech. Status reports lifecycle and exit codes only.

## Playback and UI

Foreground Talk reconnects after connection loss with a bounded 2–15 second retry
delay. Focus return allows an immediate retry. Hidden pages, sleep screens and
other modules do not reconnect. Reconnection never starts the microphone or
resends old audio. A real unavailable cloud still produces Offline.

The official desktop hello negotiates 36 frames maximum (720 ms, 46,080 bytes
of float sample data plus browser objects). Initial scheduling lead is 300 ms;
a true underrun counts once and adds 200 ms to rebuild lead. Playback credits
bound the queue. Legacy clients, the local mock and device budgets keep 12 frames.
This trades a little startup latency for tolerance of network/UI stalls, not a
guarantee against arbitrary delays. Cancel immediately stops queued sources.

MIC OFF is centered under the 78×92 character. Speaking alternates the original
open-mouth sprite and one closed-mouth frame every 140 ms; completion or cancel
returns to the usual expression. Only 125 pixels within the mouth region differ;
body, eyes, hat and identity are unchanged. This is a speaking-state animation,
not phoneme-accurate lip sync. The fourth RGB565 frame costs 14,352 Flash bytes;
there is no video or full-screen animation buffer.

The official persona now says the requested address as separate English letters:
`W W W, dot, Z C O, dot, E C O`. The saved prompt was read back exactly and Memory
was still off. This is a speech instruction, not a website deployment or domain change.

## Evidence and limits

See [machine-readable results](talk-v1/desktop-reliability.json). Synthetic-media
checks do not establish real microphone accuracy or subjective listening quality.
Actual logout/login and system-reboot acceptance remain unperformed. Physical
screen, microphone, speaker, runtime heap and battery tests are **NOT RUN**.

| Check | Result |
| --- | --- |
| Build | PASS, ESP-IDF 5.5.3; 2,756,880-byte app, 5,566,192 bytes free in factory partition |
| Host Tests | PASS, including mouth-frame reset and negotiated credit bounds |
| Browser Demo | PASS, local game/wake regression, microphone lifecycle, two mouth frames and automatic reconnect |
| Controlled playback | 250 ms injected delivery stalls and 180 ms UI stall: 240/240 frames, zero underruns, max 36 |
| Official cloud, during build | Three complete turns and interruption recovered; five underruns, smoothness not passed |
| Official cloud, after build | Three complete turns and interruption recovered; zero underruns, max queue 25, all mic tracks stopped |
| Service restart | Terminated bridge relaunched automatically; official TLS/hello passed |
| Device Tests | NOT RUN |

Build archive and matching ELF hashes are in the receipt. The initial and later
cloud results are both retained; concurrency with compilation does not establish
that compilation caused all gaps. Original site and upstream main were unchanged.

A synthetic address question was recognized correctly; the official response was
`W W W`, dot, `Z C O`, dot, `E C O`, with all 285 audio frames received.
Subjective pronunciation still needs listening acceptance.
