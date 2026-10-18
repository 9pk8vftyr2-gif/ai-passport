[English](zumi-pocket-validation.md)

# Zumi Pocket 验证报告

2026-10-18。固件源码内容已提交为 `feature/zumi-pocket` 的 `e0ea6d6`。
本次成功门禁在提交前运行，因此镜像版本为 `7753a12-dirty`；构建后未修改固件源码或显示像素。
报告及 Git 二进制属性不改变固件。

| 项目 | 状态 | 证据 |
| --- | --- | --- |
| Build | PASS | ESP-IDF 5.5.3 完整官方门禁、合并布局与匹配 ELF 归档复验通过 |
| Host Tests | PASS | 原仓库检查、内存/未定义行为检查、100,000 次游戏转换、256 次唤醒覆盖八卡、5,000 次原生/WASM 对比、素材哈希 |
| Browser Demo | PASS | Chrome 桌面/手机视口，八卡离线、相邻不重复、3 秒返回、游戏操作、完整英文标签和共用音效 |
| Device Tests | NOT RUN | 未刷写、擦除、重置或打开实体设备 |
| Unverified | 待硬件 | LCD 色彩/裁切、淡入淡出帧耗时、内部堆/LVGL 峰值、音频、整机电流与真实续航 |

## 固件身份与资源

- 应用 1,993,584 字节，factory 分区 8,323,072 字节。
- 合并镜像 2,059,120 字节；8 MB Flash、无 PSRAM。
- 八张 240×320 RGB565 宣传卡：1,228,800 字节只读 Flash。
- 基础表情 43,056 字节、PCM 20,800 字节。
- 状态 84 字节，两份绘制列表 12,808 字节；原 BSP 9,600 字节局部缓冲及 24 KB LVGL 池不变，无新增整帧解码缓冲。
- 合并镜像 SHA-256：`9299330935d98dbc80234ec55945692250cacf0dd26e61abb92d445fa3682fef`。
- 匹配 ELF SHA-256：`6c6bdbc7419ad0797b4cfcf66f3969dfd155bba9edf95c9dff453bb9835ef8d2`。
- 本地归档：`build/firmware/9299330935d98dbc80234ec55945692250cacf0dd26e61abb92d445fa3682fef/`。

归档含匹配的合并/应用镜像、ELF、MAP、bootloader、分区表、flash 参数和清单，Git 忽略且未发布。
使用 `python3 tools/archive_firmware.py verify` 加上述路径可复验。原 BSP 和分区布局不变，未混用小智 SDK。

## 实际检查

ESP-IDF 5.5.3 的 `./tools/validate.sh` 完整通过，日志为 `build/full-gate-wake-cards.log`。
首次 shell 错选 Python 3.9，随后 CMake 包装器缺少 Python 包；改用已有 IDF Python 3.12 环境及 CMake 原生执行文件解决。
最终完整门禁通过，未屏蔽检查、未更换 SDK 版本。

`tests/test_zumi.c` 另经 AddressSanitizer / UndefinedBehaviorSanitizer 检查通过，覆盖六关可解、互通线路独立判定、固定格、3,000 毫秒边界、按键消费、八卡及相邻不重复、淡入淡出边界、Garden 占位和音效。
`node tests/wasm_parity.cjs` 对比 5,000 次原生与 WASM 转换，含闲置/唤醒。
素材检查覆盖模块源码、八种原表情哈希、已审核合成图、PNG 尺寸和 RGB565 分段哈希。

`node tests/browser_zumi.cjs` 在本地资源加载后断网验证通过。证据：`build/browser-results.json`、`browser-play-title.png`、`browser-wake-card-1.png` 至 `browser-wake-card-8.png`、`browser-link.png`、`browser-wind.png`、`browser-mobile.png`。
浏览器不证明物理 LCD、电池与扬声器表现。

## 行为与边界

八张已审核 V3 宣传卡取代 Tiny Actions 清单，复用本地 Office Vibes 八种原版表情／动作，均保留 Zumi Pocket 与 zco.eco。
背景生成稿已获认可，固件未接付费 AI 服务；本地素材来源不等于微信发布认证。
固件直接读取 Flash，图像透明度避免整帧对象中间层。Garden / Talk 保留占位。
Energy Link 两处标题移至（22,16），避开电量。

仍是闲置 60 秒调暗、唤醒展示 3 秒返回首页，不是深度休眠。官方 500 mAh 电池容量不能单独证明续航，需测整机平均电流、亮度及待机状态。
未启用运动检测、一小时提醒或新休眠策略；未改 ZCO 网站、未部署 Cloudflare、未操作设备。
Fork/upstream main 均核实保持 `3d685a90bdb96dd8526d84edde1e63fec25b460e`。
参见[硬件到货验收清单](zumi-pocket.zh_CN.md#硬件到货验收清单--not-run)。
