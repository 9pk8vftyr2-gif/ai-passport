[简体中文](zumi-pocket-validation.zh_CN.md)

# Zumi Pocket Validation

Validation date: 2026-10-18. Scope: phase-one software prototype, without hardware.

| Field | Status | Evidence |
| --- | --- | --- |
| Build | NOT RUN | ESP-IDF 5.5.3 toolchain preparation in progress; updated after final build |
| Host Tests | PASS | Shared repository static gate; Zumi native tests with AddressSanitizer/UndefinedBehaviorSanitizer; 100,000 randomized transitions; 5,000 native/WASM replay comparisons |
| Browser Demo | PASS | Chrome/Chromium desktop 1440 x 1000 and mobile viewport 390 x 844; ten interaction checks; offline gameplay; no page errors |
| Device Tests | NOT RUN | Hardware has not arrived; no flashing or storage operations performed |
| Unverified | OPEN | Physical LCD/font appearance, ADC timing, button latency, long-run heap/fragmentation, real battery readings/current, board boot and owner artwork acceptance |

## Reproduce

- `./tools/validate.sh --static`: repository rules, workflow lint, baseline/BSP
  host tests and added Zumi tests; `build/static.log` holds the local transcript.
- `cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Imain/zumi tests/test_zumi.c main/zumi/core.c main/zumi/view.c -o build/test_zumi`
  then `build/test_zumi`: meaningful game/navigation paths and randomized invariants.
- `node tests/wasm_parity.cjs`: 5,000 replay comparisons of the native model and
  checked-in browser module. Artifact hashes prevent a stale module passing the
  static gate after a model or view change.
- `node tests/browser_zumi.cjs`: running local preview required; Playwright and
  Chrome required. Evidence: `build/browser-results.json`, `build/browser-home.png`,
  `build/browser-carbon.png`, `build/browser-journey.png`, `build/browser-mobile.png`.
- The asset compiler preserves the source vector and records SHA-256 provenance.
  No image generation service or external AI service is called.

The browser test covers home, both unavailable-service screens, Carbon Pop
scoring, long-press release suppression, Journey entry/jump, offline play,
loss/replay, mobile overflow and physical-button controls. A mobile viewport is
not a physical-phone test. Safari and real mobile browsers are not verified.

The repository gate needed two scoped macOS portability fixes: use `shasum` for
its pinned actionlint checksum because macOS `sha256sum` lacks GNU long options,
and use the Darwin linker's `-dead_strip` for existing demo runtime tests.
Linux retains `--gc-sections`. No baseline test was removed or suppressed.

## Environment and delivery boundary

Independent checkout, outside the ZCO website repository. Required five passport
skills are installed as repository-local links and verified readable. The official
ESP-IDF v5.5.3 annotated tag is `b31fcc7a314a44ad992b58f589f7d1d8a4fadff6`;
its peeled source commit is `2c211b236707889e8400c4dc5644dd5c4ee071e0`.
No alternate Xiaozhi SDK is used. wasi-sdk 34 builds only the browser module.

Only the requested feature branch is published to the selected fork. Upstream
main, fork main, the existing ZCO website and Cloudflare remain untouched.
See the [hardware-arrival checklist](zumi-pocket.md#hardware-arrival-checklist--not-run).
