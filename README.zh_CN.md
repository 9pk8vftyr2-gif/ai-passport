[English](README.md)

# Zumi Pocket

面向 FoloToy AI Passport 的第一阶段离线原型：ESP32-C3、8 MB Flash、无 PSRAM、240 × 320 竖屏。Zumi 是男孩，使用现有官方角色素材，仅调整面部表情，不重新设计外观。

## 浏览器预览

在仓库根目录运行 `python3 -m http.server 8876 --bind 127.0.0.1`，打开 <http://127.0.0.1:8876/preview/>。预览资源已提交，无须安装依赖。使用屏幕下方三键或键盘 ↑ / ↓ / Enter；长按 OK 500 毫秒回首页。屏幕无需触摸，资源加载后可离线游玩。

## 第一阶段

- 首页：Talk / Play / Garden。
- 唯一游戏 **Energy Link**：延续网站旋转线路、点亮全部房屋的规则，重新编排 6 个 3×3 口袋关卡。UP / DOWN 循环选择可旋转线路，OK 顺时针旋转。Solar power / Wind power 表示固定能源，House 表示固定房屋，相对接头连接才通电。没有时间或步数限制。完成后保留棋盘，OK 进入下一关准备页；准备页上下选关。
- 音效沿用网站 Energy Link 的旋转、通电和通关声音，浏览器与固件播放同一份 16 kHz PCM。音频失败不影响游戏；实体扬声器响度与音色待真机验证。
- 所有页面无操作 60 秒自动关闭背光并停止应用重绘，按键恢复亮度并随机展示 8 张已审核宣传卡之一，相邻两次不重复。八张复用不同原版 Zumi 表情／动作，保留 Zumi Pocket 和 zco.eco。唤醒按键被消费；3 秒后淡出进入首页。游戏页也遵循此规则。属于熄屏待机，未进入深度休眠；实际省电量待测。
- 宣传卡取代原 Tiny Actions 行动清单，Garden 与 Talk 恢复预留占位入口，无完成计数、联网、录音、付费 AI 或存储写入。
- Energy Link 准备页与游戏页标题均移至左上角（22,16），避开电量。
- 按用户选择，运动检测与一小时站立提醒等硬件确认后再做；本版不启用未按键计时替代。

固件与浏览器编译同一套 C 逻辑和绘制列表，分别由原 BSP 的 LVGL 和浏览器 Canvas 绘制。预览不能代替真机显示与性能验证。官方测试菜单源文件仅保留作参考，不链接到应用入口。

## 开发验证

严格使用 ESP-IDF **5.5.3**，不混用小智仓库 SDK。五个官方 Skills 通过 `python3 tools/install_passport_skills.py --install` 安装，并用 `--check` 检查。

激活 SDK 后运行 `./tools/validate.sh` 执行完整检查；`--static` 只检查静态与主机测试，`--firmware` 只构建并校验合并镜像。这些操作不刷机。镜像及匹配 ELF/MAP 保存在 `build/firmware/`。

浏览器重新编译：`WASI_SDK_PATH=/path/to/wasi-sdk-34 ./tools/build_preview.sh`。开发依赖用 `pnpm install --frozen-lockfile` 安装。`node tests/wasm_parity.cjs` 比较 5,000 次原生/WASM 状态；运行预览服务后用 `node tests/browser_zumi.cjs` 测试 Chrome（可设 `PLAYWRIGHT_CHANNEL`）。结果和截图在忽略目录 `build/`。

`node tools/build_zumi_assets.cjs` 使用 Sharp 编译现有素材；图片来源与校验值在 `assets/images/zumi-pocket/`。生成的 C 资源已提交，固件构建不需要 Node。

参见[设计与硬件验收清单](docs/assets/zumi-pocket.zh_CN.md)和[构建测试报告](docs/assets/zumi-pocket-validation.zh_CN.md)。

## 分支边界

Fork：`9pk8vftyr2-gif/ai-passport`；分支：`feature/zumi-pocket`。上游基线 `3d685a90bdb96dd8526d84edde1e63fec25b460e`。
不改上游或 Fork main，不改现有 ZCO 网站，不部署 Cloudflare，不刷写、擦除设备或接入付费服务。

宣传卡转换：`node tools/build_zumi_wake_cards.cjs`（Sharp），8 张 240×320 RGB565 共占 1,228,800 字节 Flash，运行时不申请整帧解码缓冲。
