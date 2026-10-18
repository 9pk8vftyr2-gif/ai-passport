<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

Zumi Pocket uses [the existing ivory 2D vector](images/zumi-pocket/zumi-original.svg)
and three 78 x 92 PNG derivatives (`zumi-0.png` through `zumi-2.png`) under
`images/zumi-pocket/`. `manifest.json` records source and derivative hashes.
Zumi is a boy; only face expressions are varied. The source is ZCO's supplied
character artwork, with no additional third-party redistribution license granted.
Run `node tools/build_zumi_assets.cjs` from the repository root (Sharp required)
to compile 43,056 bytes of RGB565 data into `main/zumi/sprites.c`; browser and
firmware use the same small pictures. See the [design notes](../docs/assets/zumi-pocket.md).

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Zumi Pocket uses `music/zumi-link-tones.json` as the editable source for the
existing ZCO Energy Link synthesized sine tones (source revision recorded there).
No recordings or third-party audio assets are copied. Run
`python3 tools/build_zumi_sounds.py` to compile `main/zumi/sounds.c`: three
16 kHz, mono, signed 16-bit PCM clips, 20,800 bytes in Flash, streamed in
512-byte chunks by the original BSP. The generated data follows this repository's
code license. Browser and firmware use the same PCM. Device sound is unverified.


Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.

### Approved promotional wake cards

`images/zumi-pocket/wake-cards/` holds eight approved 240 x 320 RGB565-quantized PNGs and the concatenated `wake_cards.rgb565` (1,228,800 bytes). `source/` contains the approved V3 composites and original sticker identity hashes from the owner-supplied local Zumi: Office Vibes package. This does not assert WeChat publication. Built-in image generation edited backgrounds; original sticker layers were reused without redrawing. Owner approval for device integration was given on 2026-10-18. Run `node tools/build_zumi_wake_cards.cjs` (Sharp) to reproduce. The native binary is linked read-only from Flash; the browser uses matching quantized PNGs. No runtime PNG/JPEG decode or full-frame buffer. The same ZCO artwork licensing boundary applies.
