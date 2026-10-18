[English](zumi-pocket-validation.md)

# Zumi Pocket 验证报告

日期：2026-10-18。固件源码：`feature/zumi-pocket` 的 `6a95860`。范围：硬件到货前的第一阶段软件原型。

| 项目 | 状态 | 证据 |
| --- | --- | --- |
| Build | PASS | ESP-IDF v5.5.3、ESP32-C3 官方完整检查通过，合并镜像和匹配 ELF 校验通过 |
| Host Tests | PASS | 仓库静态与原有基线测试；六关可解；8 张行动卡完成/去重/重置/跨页保留；10 万次状态转换与独立连通性校验；音效事件、PCM 幅度/边界及通关延时；地址/未定义行为检查；5,000 次原生/WASM 对照 |
| Browser Demo | PASS | Chrome 桌面 1440×1000、手机视口 390×844，23 项交互检查通过，离线游玩、无页面错误，截图已人工查看 |
| Device Tests | NOT RUN | 硬件未到；没有刷写、擦除或重置设备 |
| Unverified | OPEN | 实体扬声器响度/音色/失真、音频失败恢复与显示并发、实际屏幕与字体、淡入淡出顺滑度、按键延迟、LVGL 分配峰值、长时间碎片、电量/电流/温度、Safari、实体手机及用户视觉验收 |

## 构建证据

应用镜像 765,456 字节，factory 分区 8,323,072 字节；合并镜像 830,992 字节。原 BSP 和分区表未修改，无 PSRAM 依赖。新增 Montserrat 10 网址小字，没有增加存储格式或写入。新增音频任务通过原 BSP 分块播放，复用网站音效参数，PCM 占 Flash 20,800 字节。

合并镜像 SHA-256：
`5048a5a3749b3f9f9a80d355250cdd03f5f536f27ea2221eeaecad0da5757e7e`。
匹配 ELF SHA-256：
`0a725f0edd0f7fe493e05ba4976dd214d72a0ca0ed04214127df42988c1839ae`。

本地镜像/ELF/MAP/manifest 保存在 `build/firmware/5048a5a3749b3f9f9a80d355250cdd03f5f536f27ea2221eeaecad0da5757e7e/`。
应用描述确认源码版本 `6a95860`、IDF `v5.5.3`。构建产物被 Git 忽略，并非发布的固件版本。最终报告提交不改变已测试的固件、浏览器模块或素材。

## 复现与测试范围

- 激活 ESP-IDF 5.5.3 后运行 `./tools/validate.sh`。日志：`build/full-gate-cards.log`，原基线检查全部保留。
- `cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Imain/zumi tests/test_zumi.c main/zumi/core.c main/zumi/view.c main/zumi/sound.c main/zumi/sounds.c -o build/test_zumi`，再运行 `build/test_zumi`：通过操作解六关，检查固定格、无限时、下一关循环、三键唤醒与释放抑制、精确 3,000 毫秒欢迎页、首页淡入、网址和绘制边界。随机状态使用独立的双向接头遍历校验。
- `node tests/wasm_parity.cjs`：原生 C 与 WASM 的 5,000 次状态对照。源码/模块校验值防止旧模块冒充更新。
- 预览服务器运行后执行 `node tests/browser_zumi.cjs`：首页、Talk、Garden、游戏入口、第一关通关、下一关、离线旋转、无限时思考、长 OK 释放、空闲变暗、不同表情、网址、3 秒返回、手机视口溢出、实体按键样式控件、完整英文标签、真实 Web Audio PCM 播放、退出停止声音、卡片浏览、离线完成 8 张卡、防重复计数、跨首页保留、新组重置，共 23 项。
- 浏览器证据位于 `build/`：`browser-results.json`、`browser-home.png`、`browser-link.png`、`browser-link-complete.png`、`browser-wake.png`、`browser-mobile.png`、`browser-wind.png`、`browser-actions.png`、`browser-action-done.png`、`browser-actions-complete.png`、`browser-actions-mobile.png`。手机视口不等于实体手机验证。
- 素材编译保留来源哈希，没有调用图像生成或付费服务。浏览器无法验证实际 LVGL 内存。

官方检查做了两处 macOS 兼容修复：actionlint 校验使用 `shasum`；原有主机 demo 测试用 Darwin 的 `-dead_strip`，Linux 仍用 `--gc-sections`。没有删减或跳过基线测试。

## 环境与交付边界

五个官方 passport Skills 已安装并检查可读。ESP-IDF v5.5.3 tag 对象为 `b31fcc7a314a44ad992b58f589f7d1d8a4fadff6`，源码提交 `2c211b236707889e8400c4dc5644dd5c4ee071e0`；未混用小智 SDK。wasi-sdk 34 仅构建浏览器。角色来源见 `assets/images/zumi-pocket/manifest.json`。

独立仓库、Fork `9pk8vftyr2-gif/ai-passport`，只更新 feature 分支。上游与 Fork main 均核对为 `3d685a90bdb96dd8526d84edde1e63fec25b460e`。ZCO 网站仅只读参考 Energy Link 规则，未修改网站、部署 Cloudflare、接入付费 AI 或操作设备。

本次仅新增行动卡。运动检测及一小时站立提醒已按用户选择延后，等硬件确认后再做；没有启用每小时计时或未按键计时替代。行动卡完成是用户自报，本次开机内保留，重启清空，无联网或存储写入。

参见[硬件到货验收清单](zumi-pocket.zh_CN.md)。
