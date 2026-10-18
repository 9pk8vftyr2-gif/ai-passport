[简体中文](zumi-talk-persona.zh_CN.md)

# Zumi hosted role and voice acceptance

2026-10-18. At the user's request, the existing bound official XiaoZhi desktop
agent was renamed Zumi Pocket and configured through the signed-in Chrome
console. No login credentials or account identifiers are included here.

## Applied and read back

- Custom Zumi role: the saved text exactly matched the Chinese prompt in
  [zumi-persona.json](../../config/zumi-persona.json).
- Voice: Sunshine Youth male preset (the original Chinese label is retained in
  the configuration and receipt). Reply speed 1; pitch 0. Natural delivery is
  requested in the role, without artificially raising pitch or accelerating it.
- Model: existing Xiaozhi Lite, unchanged. No paid plan or service was selected.
- Memory: previously on, switched off, saved, and verified off after reopening.
  No history or previously stored memories were deleted. This setting does not
  establish platform retention or erasure policy.

The prompt preserves the original character, avoids a boy/child identity, limits
answers to short conversational replies, and discloses the real backend when
asked. It does not claim website-account access, Garden control or live browsing.

Website facts were checked against the live [Zumi page](https://zco.eco/zumi/)
and [About page](https://zco.eco/about/): community purpose, stickers and editable
art, Guess Carbon and Green Journey, CarbonView, CarbonLedger, EnergyPlan for
Island, and the boundary between participation scores and verified reductions.
These are dated knowledge facts, not automatic retrieval of future website changes.

## Actual cloud tests

Five public Chinese questions were synthesized locally and streamed through the
existing desktop PCM-to-Opus bridge. Each used a fresh official session. All five
returned recognized text, answer text and complete playable PCM audio.

| Case | Observed answer behavior | Result |
| --- | --- | --- |
| Self-introduction | Introduced Zumi and its Chinese name as ZCO's green-living companion in Zumi Pocket | PASS |
| Website relationship | Explained ZCO as a community combining games, tools, actions and creation; distinguished character and pocket product | PASS |
| Website tools | Correctly described CarbonView, CarbonLedger and EnergyPlan; preserved the engineering-design limitation | PASS |
| Capability and climate claims | Rejected equating game scores with real carbon reductions and denied unavailable Garden control | PASS |
| Current website updates | Admitted inability to confirm live updates and directed the user to the website | PASS |

The [receipt](talk-v1/persona-acceptance.json) includes exact public test questions,
ASR results, replies and frame counts: 3,387 returned PCM frames in total. ASR
misspelled the brand abbreviation in two questions and a carbon-reduction term
in another; answer semantics remained correct. This is a sampled acceptance test,
not a guarantee of every future answer or of recognition with a human microphone.

Generated test input and returned voice samples remain in ignored local
`build/talk-persona/`. They are not embedded in firmware or redistributed in Git.
The ordinary runtime still does not save conversations; this explicit test saved
only its own public synthetic questions and responses for review.

## Validation boundary

Build: NOT RUN for this configuration-only change; firmware is unchanged.
Host Tests: PASS for the repository static/host gate.
Browser Demo: PASS for configuration save/readback and local setup-page sync.
Cloud voice tests: PASS, five complete question/answer streams.
Device Tests: NOT RUN; no flash, reset, erase or serial use.
Unverified: subjective voice quality, human microphone/acoustics, hosted retention,
and all physical-device performance and direct-cloud memory requirements.

Re-enter Talk at <http://127.0.0.1:8876/preview/?talk=xiaozhi> to start with the
saved settings. The original offline game and wake flow were not changed.
