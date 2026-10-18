[English](zumi-pocket-validation.md)

# Zumi Pocket 验证报告

2026-10-18。`feature/zumi-pocket` 的首页/初始选关页重设计与直接下一关增量，基于 `20babf7`。
成功门禁在提交前运行，镜像版本为 `20babf7-dirty`；构建后未修改固件源码或显示像素。
本报告中的合并镜像和 ELF 哈希标识本次实际产物。

| 项目 | 状态 | 证据 |
| --- | --- | --- |
| Build | PASS | ESP-IDF 5.5.3 完整官方门禁、合并布局与匹配 ELF 归档复验通过 |
| Host Tests | PASS | 原仓库检查、内存/未定义行为检查、100,000 次游戏转换、各页面 60 秒边界/操作重置/迟到按键防误触、256 次唤醒覆盖八卡、5,000 次原生/WASM 对比、素材哈希 |
| Browser Demo | PASS | Chrome 桌面/手机视口，新首页、六关缩略图无角色、通关后直接下一关、黑屏像素、游戏熄屏唤醒并原样继续、八卡离线、相邻不重复、3 秒返回、游戏操作、完整英文标签和共用音效 |
| Device Tests | NOT RUN | 未刷写、擦除、重置或打开实体设备 |
| Unverified | 待硬件 | 实际背光关闭与按键唤醒延迟、LCD 色彩/裁切、淡入淡出帧耗时、内部堆/LVGL 峰值、音频、整机电流与真实续航 |

## 固件身份与资源

- 应用 1,994,432 字节，factory 分区 8,323,072 字节。
- 合并镜像 2,059,968 字节；8 MB Flash、无 PSRAM。
- 八张 240×320 RGB565 宣传卡：1,228,800 字节只读 Flash。
- 基础表情 43,056 字节、PCM 20,800 字节。
- 状态 88 字节，两份绘制列表 12,808 字节；原 BSP 9,600 字节局部缓冲及 24 KB LVGL 池不变，无新增整帧解码缓冲。
- 合并镜像 SHA-256：`c6cbbcdd92ba20af2a8e161aa502c4917be78d5fe982dea9e33ce04ec227b662`。
- 匹配 ELF SHA-256：`ec34af33b5fcd69509d65a958a3306760cc11c26c389a3afc5a423515987ce45`。
- 本地归档：`build/firmware/c6cbbcdd92ba20af2a8e161aa502c4917be78d5fe982dea9e33ce04ec227b662/`。

归档含匹配的合并/应用镜像、ELF、MAP、bootloader、分区表、flash 参数和清单，Git 忽略且未发布。
使用 `python3 tools/archive_firmware.py verify` 加上述路径可复验。原 BSP 和分区布局不变，未混用小智 SDK。

## 实际检查

ESP-IDF 5.5.3 的 `./tools/validate.sh` 完整通过，日志为 `build/full-gate-layout-direct-next.log`。


`tests/test_zumi.c` 另经 AddressSanitizer / UndefinedBehaviorSanitizer 检查通过，覆盖六关可解、互通线路独立判定、固定格、3,000 毫秒边界、按键消费、八卡及相邻不重复、淡入淡出边界、Garden 占位和音效。
`node tests/wasm_parity.cjs` 对比 5,000 次原生与 WASM 转换，含闲置/唤醒。
素材检查覆盖模块源码、八种原表情哈希、已审核合成图、PNG 尺寸和 RGB565 分段哈希。

`node tests/browser_zumi.cjs` 在本地资源加载后断网验证通过。证据：`build/browser-results.json`、`browser-screen-off.png`、`browser-game-resumed.png`、`browser-play-title.png`、`browser-wake-card-1.png` 至 `browser-wake-card-8.png`、`browser-link.png`、`browser-wind.png`、`browser-mobile.png`。
浏览器不证明物理 LCD、电池与扬声器表现。

## 行为与边界

八张已审核 V3 宣传卡取代 Tiny Actions 清单，复用本地 Office Vibes 八种原版表情／动作，均保留 Zumi Pocket 与 zco.eco。
背景生成稿已获认可，固件未接付费 AI 服务；本地素材来源不等于微信发布认证。
固件直接读取 Flash，图像透明度避免整帧对象中间层。Garden / Talk 保留占位。
Energy Link 两处标题位于（22,16），避开电量。首页改为三个编号功能入口，移除角色；初始选关页中央改为对应关卡线路缩略图。
六关缩略图检查通过，绘制指令为 28–43 个，低于 64 个上限，无新增图片资源。
通关后短按 OK 直接进入下一关棋盘，第六关后回第一关；不再经过中间选关页。初次从 Play 进入仍可自由选关。切关时取消上一关音效。
截图：`build/browser-home-redesign.png`、`build/browser-level-preview-1.png` 至 `browser-level-preview-6.png`。

所有页面无操作 60 秒后关闭背光（70% → 0%）并停止应用重绘。
任意键唤醒，随机卡片展示 3 秒后回到原游戏；从其他页面唤醒则回首页。
保留关卡、线路、步数、光标和通关状态，恢复页面淡入 240 毫秒；下一次 60 秒计时从恢复页面开始。
新增测试覆盖六关 × 三键 × 三次连续熄屏恢复、3 秒边界、长按唤醒防误触、通关画面保留、继续旋转和下一关。
测试覆盖超时边界、操作重置计时、所有三键唤醒、迟到长按/释放防误触和反复唤醒。
唤醒帧在背光恢复前更新，仍需实机检查是否闪现旧画面。熄屏时取消待播音效，但不关闭已初始化音频硬件。
CPU、LCD 控制器和按键轮询继续工作；这是熄屏待机，不是深度休眠。
官方 500 mAh 电池容量不能单独证明续航，需分别测亮屏、熄屏及游戏的整机电流。
未启用运动检测或一小时提醒；未改 ZCO 网站、未部署 Cloudflare、未操作设备。
Fork/upstream main 均核实保持 `3d685a90bdb96dd8526d84edde1e63fec25b460e`。
参见[硬件到货验收清单](zumi-pocket.zh_CN.md#硬件到货验收清单--not-run)。
