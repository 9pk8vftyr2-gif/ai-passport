<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

Zumi Pocket 使用[现有米白 2D 矢量](images/zumi-pocket/zumi-original.svg)，以及
`images/zumi-pocket/` 下三张 78×92 PNG 衍生图 `zumi-0.png` 至 `zumi-2.png`。
`manifest.json` 记录源文件与衍生图哈希。Zumi 是男孩，仅改变面部表情。
素材来自 ZCO 提供的角色形象，不额外授予第三方再分发许可。
在仓库根目录运行 `node tools/build_zumi_assets.cjs`（需要 Sharp），
将 43,056 字节 RGB565 数据编译至 `main/zumi/sprites.c`；浏览器和固件使用相同小图。
详见[设计说明](../docs/assets/zumi-pocket.zh_CN.md)。

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

Zumi Pocket 使用 `music/zumi-link-tones.json` 记录现有 ZCO Energy Link 合成正弦音参数和来源版本，不复制录音或第三方音频。运行 `python3 tools/build_zumi_sounds.py` 生成 `main/zumi/sounds.c`：三份 16 kHz、单声道、有符号 16 位 PCM，共占 Flash 20,800 字节，由原 BSP 按 512 字节分块播放。生成数据沿用仓库代码许可。浏览器与固件共用 PCM，真机听感尚未验证。


可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。

### 已审核唤醒宣传卡

`images/zumi-pocket/wake-cards/` 保存 8 张 240×320 RGB565 量化 PNG 和合并原始像素 `wake_cards.rgb565`（1,228,800 字节）。`source/` 保留已审核 V3 合成图及用户本地 Zumi: Office Vibes 原表情哈希，不声称已在微信发布。内置图片生成仅编辑背景，角色图层复用原文件；用户于 2026-10-18 同意接入固件。执行 `node tools/build_zumi_wake_cards.cjs`（Sharp）可复现。固件只读访问 Flash，浏览器显示相同量化 PNG；运行时无 PNG/JPEG 解码或整帧缓冲。沿用上述 ZCO 角色许可边界。
