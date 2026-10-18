[English](zumi-pocket-validation.md)

# Zumi Pocket 验证报告

验证日期：2026-10-18。范围：无硬件条件下的第一阶段软件原型。

| 字段 | 状态 | 证据 |
| --- | --- | --- |
| Build | NOT RUN | 正在准备 ESP-IDF 5.5.3 工具链，最终构建后更新 |
| Host Tests | PASS | 仓库完整静态门禁；Zumi 原生测试及 AddressSanitizer/UndefinedBehaviorSanitizer；10 万次随机状态转换；5,000 次原生/WASM 对照 |
| Browser Demo | PASS | Chrome/Chromium 桌面 1440×1000、手机视口 390×844；十项交互；离线游戏；无页面错误 |
| Device Tests | NOT RUN | 硬件尚未到货，未执行刷写或设备存储操作 |
| Unverified | OPEN | 真屏/字体、ADC 时序、按键延迟、长时间堆内存与碎片、电量/电流、真机启动及角色视觉验收 |

## 复现方法

- `./tools/validate.sh --static`：仓库规范、工作流检查、原有 BSP/主机测试及新增
  Zumi 测试；本地记录见 `build/static.log`。
- `cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Imain/zumi tests/test_zumi.c main/zumi/core.c main/zumi/view.c -o build/test_zumi`
  然后执行 `build/test_zumi`，覆盖游戏/导航实际分支及随机不变量。
- `node tests/wasm_parity.cjs`：对照原生模型和已提交浏览器模块的 5,000 次回放。
  静态门禁检查产物哈希，避免模型或视图修改后仍使用过期模块。
- `node tests/browser_zumi.cjs`：需要本地预览服务、Playwright 和 Chrome。
  证据为 `build/browser-results.json`、`build/browser-home.png`、
  `build/browser-carbon.png`、`build/browser-journey.png`、`build/browser-mobile.png`。
- 资源编译器保留原始矢量，记录 SHA-256 来源。未调用图像生成或外部 AI 服务。

浏览器测试覆盖首页、两个未接入服务页、Carbon Pop 计分、长按抬起抑制、
Journey 进入/跳跃、离线游玩、失败/重玩、手机视口溢出和实体按键模拟控件。
手机视口不等于真机手机验收；Safari 和真实移动浏览器未验证。

官方门禁做了两处有界 macOS 兼容修复：macOS `sha256sum` 不支持 GNU 长选项，
改用 `shasum` 校验固定版本 actionlint；原 demo 测试在 Darwin 使用
`-dead_strip`，Linux 保留 `--gc-sections`。未删除或跳过原有测试。

## 环境与交付边界

使用 ZCO 网站仓库之外的独立目录。五个 passport 技能已建立仓库本地链接并确认可读。
官方 ESP-IDF v5.5.3 的注解标签对象为 `b31fcc7a314a44ad992b58f589f7d1d8a4fadff6`，
实际源码提交为 `2c211b236707889e8400c4dc5644dd5c4ee071e0`，未使用小智替代 SDK。
wasi-sdk 34 仅用于浏览器模块。

仅向选定 Fork 发布请求的功能分支；上游 main、Fork main、原 ZCO 网站与 Cloudflare
均不变。详见[硬件到货清单](zumi-pocket.zh_CN.md)。
