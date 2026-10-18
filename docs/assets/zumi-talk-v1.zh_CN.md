[English](zumi-talk-v1.md)

# Zumi Talk V1.0：架构审计与设计

日期：2026-10-18。审计基线：`feature/zumi-pocket` 的 `b61f8c0`。
本阶段交付固件审计、内存预算、可执行的六状态 C 模型及 240×320 浏览器交互设计。
**没有接入硬件录音、Wi-Fi 连接、ASR/LLM/TTS 服务**。状态模型未加入 `main/CMakeLists.txt`，现有固件中的 Talk 占位入口、Zumi 形象、小游戏和唤醒逻辑保持原样。
未刷写设备、操作设备存储、启用收费服务或修改网站。

[打开 Talk 交互预览](http://127.0.0.1:8876/preview/talk/) · [审计数据](talk-v1/audit.json) · [测试报告](zumi-talk-v1-validation.zh_CN.md)

## 1. 当前固件审计

以下是已核验本地固件产物的数据，不代表尚未读取的实体设备内容。

| 分区 | 偏移 | 字节 | 用途 |
| --- | --- | ---: | --- |
| NVS | `0x9000` | 24,576 | 配置区；不擦除、不重置 |
| PHY | `0xf000` | 4,096 | 无线校准数据 |
| factory | `0x10000` | 8,323,072 | 单应用分区；没有 OTA 槽或文件系统分区 |

- Flash 配置：8,388,608 字节，无 PSRAM；分区表位于 `0x8000`。
- 应用固件：**2,006,896 字节（1.914 MiB）**。
- 应用分区剩余：**6,316,176 字节（6.023 MiB）**，实际并非约 3MB。
- 合并镜像：2,072,432 字节；含引导和填充，不应把整个合并镜像从应用分区容量中扣除。
- 合并镜像 SHA-256：`cb681e7452c32b843511904bff3760614e3aac0f3123296854623f2f94aadd2a`。
- 匹配 ELF SHA-256：`34e33e22cbaf63ab37d335075f6ea558b17488fb3c6d3f1003cdab5b21c16ddc`。
- 镜像版本：`1c7a2cf-dirty`，ESP-IDF `v5.5.3`；这是 `b61f8c0` 提交前构建的对应产物。
- 链接报告：共享内部内存区域 321,296 字节；已用 117,484（驻留代码 67,320、BSS 42,792、data 7,372），链接剩余 **203,812 字节**。
- **链接剩余不等于可用 Heap**。任务、DMA、驱动和分配器继续占用内存，碎片还影响连续分配。
- 运行时可用内部 Heap、最低可用 Heap、最大连续 Heap：**未测量 / null**。本次没有设备会话，不能声称语音已通过内存准入。

### 驱动与分配核查

| 项目 | 当前源码 | 审计结果与影响 |
| --- | --- | --- |
| ES8311 / I2S | `components/bsp/src/bsp_audio.c` | BSP 独占共享 I2C；应用 PCM 为 16kHz、16位、单声道，物理 I2S 两时隙；已有双向通道，Talk V1 采用半双工 |
| I2S DMA | 同上 | 每方向 6×240 帧；按两时隙 PCM 计算每方向载荷 5,760 字节、合计 11,520，另有元数据；实际分配待打开驱动后测量 |
| 阻塞时间 | BSP 与 codec-dev I2S 平台层 | `bsp_audio_read/write` 阻塞，依赖中的等待上限为 1,000ms；20ms 小块不代表可在 20ms 内终止硬件读取 |
| 游戏音效 | `main/zumi/audio.c` | 4,096 字节任务栈，512 字节 PCM 栈内缓冲，单项队列，惰性初始化；目前取消没有工作任务完成确认 |
| 显示 | `bsp_display_lvgl.c` | ST7789 SPI、LVGL 9.5；**40 行 RGB565 单缓冲 = 19,200 字节**，静态 LVGL 池 24,576 字节；旧 Zumi 报告误写 9,600，本次纠正 |
| 状态与绘制 | `main/zumi_main.c` | 两份各 6,404 字节绘制列表，游戏状态 104 字节；回调入队，主循环加 LVGL 锁 |
| Wi-Fi | `main/CMakeLists.txt`、`main/demo_wifi.c` | Zumi 未链接运行 Wi-Fi；无线驱动是 ESP-IDF 能力，并没有现成的 `bsp_wifi` 接口。官方主 demo 只有扫描，STA/配网生命周期需补充 |
| 素材 | 当前素材清单 | 唤醒卡 1,228,800、基础形象 43,056、音效 20,800、Eco Pop 图标 12,288 字节，位于只读 Flash，全部保留 |
| 字体 | `sdkconfig.defaults` | Montserrat 10/14/20，不具备通用中文字形；浏览器中文字不等于设备已支持 |

游戏打开 ES8311 IN/OUT 时，ADC 可能保持开启；当前没有应用麦克风读取循环或语音上传。
未来 Talk 的隐私边界是：Listening 以外不读取、不上传录音；无人使用音频时执行软件挂起。
不能宣称物理断麦克风电源，codec/功放供电没有软件开关。

## 2. 四个开源项目研究

| 仓库与固定版本 | 借鉴内容 | 不直接移植的内容 |
| --- | --- | --- |
| [FoloToy/ai-passport · 3d685a9](https://github.com/FoloToy/ai-passport/tree/3d685a90bdb96dd8526d84edde1e63fec25b460e) | 当前 BSP 所有权、codec 恢复、LVGL 锁、工作任务退出约束 | 官方硬件测试菜单、录音 demo 的 96KB 整段缓冲 |
| [FoloToy/folo-ai-passport-xiaozhi · 72da544](https://github.com/FoloToy/folo-ai-passport-xiaozhi/tree/72da544d1b51678f5d88967adc299b3aec956946) | 协议抽象、listen/stop/abort、流式事件 | 整套固件、分区/OTA、唤醒词/AEC、默认服务端。当前 README 要求 IDF ≥6.0.1，推荐 6.1；Zumi 仍使用 5.5.3 |
| [zhaohuaxiaoy/folo-ai-passport-voice · 403ea8e](https://github.com/zhaohuaxiaoy/folo-ai-passport-voice/tree/403ea8e244a3862e0018ef41376e32ab79062da3) | 纯 C 状态模型、会话 token 作废、小缓冲、采集与传输分工、先排空再结束 | 桌面文字注入、审批页、绑定服务商、照抄缓冲参数。源码使用 3,200 字节帧和 6,432 字节环形缓冲，并可选 ADPCM |
| [linysky/hermes-passport · 6bcf56c](https://github.com/linysky/hermes-passport/tree/6bcf56c64c27162281b2a5efe6686c7e9a8e2765) | BLE 伴侣端边界、请求/会话/序列号、文本分页 | 多机器人、历史记录；`main/hermes_ble.c` 收到音频后的 TTS 播放仍为未来占位，不能视为完整播放实现 |

四个仓库顶层许可证均为 MIT，组件许可证需分别遵守。本阶段未复制第三方实现。
重点核查：小智的 `main/protocols/protocol.h`、`websocket_protocol.cc`、`main/audio/audio_service.h`；Voice 的 `main/audio_streamer.c`、`app_protocol.h`；Hermes 的 `main/hermes_ble.c`、`hermes_bridge.c`、`text_layout.h`、`adpcm.h`。README 描述不代替代码和硬件验证。

另检查官方 [demo/blufi-provisioning · 9c039cc](https://github.com/FoloToy/ai-passport/tree/9c039cc5127f22072afa83bedb7fa3d8efe635ad) 的配网、安全回调与无线生命周期。
后续只提取连接与凭据处理逻辑，保留安全回调，不整分支合并，不创建第二套 I2C/ADC。
配网需明确进入，不打印密码、不因错误擦除 NVS；Talk 前停止 BLE，避免 BLE/Wi-Fi 同时运行的内存峰值。

## 3. 建议架构

```text
三键 → 现有输入队列 → Talk 控制器（纯状态 / 事件）
                         ├─ UI 快照 → 现有 LVGL
                         └─ 可替换传输适配器
                              ↕ 有界上下行队列
                           单一音频管理者
                              ↕
                         BSP ES8311 / I2S

设备 ← WSS / 小块 PCM → 可替换中继服务
                           ├─ ASR 适配器
                           ├─ LLM 适配器
                           └─ TTS 适配器
```

V1 优先 **16kHz、单声道、PCM16 小端，20ms / 640 字节一帧**：32,000 字节/秒，即协议开销前 256kbit/s。
中继统一服务商的采样率、音频容器和文本。先用 Wi-Fi，避免给 ESP32-C3 同时增加 Opus 编解码器。
如果 Wi-Fi/TLS 实测内存不够，再评估 BLE/USB 伴侣端适配器；不直接换成完整小智固件。

**“录音结束后发送”按“结束后提交问答”设计**：为避免设备保存完整录音，Listening 时小块音频就上传到临时中继会话，可流式识别；只有 `input.commit` 后才发起 LLM/TTS。取消则删除未提交会话，预览页面已说明。
如果要求结束前一字节都不出设备，需要另选 Flash 临时存储或明显缩短录音时长；本方案没有偷偷采用这两者。

### 后续接入位置与所有权（尚未启用）

- 保留 `zumi_main.c` 单一状态/UI 管理者，增加 Talk 事件分发，不重写导航和游戏。新增 `talk_model.c` 不依赖 IDF/LVGL，本阶段不链接进固件。
- 音频管理者统一分配游戏音效和 Talk。录音前取消游戏音效并等待工作任务确认；串行管理初始化、采样格式、读写、挂起和恢复，不释放仍被使用的资源。
- Wi-Fi、TLS/设备鉴权、协议协商和音频管理者就绪后才能进入可录音的 Idle；失败保持 Offline/Error。
- 半双工：Listening 只读，Processing 不采集，Speaking 只写。打断时停止播放、清队列、作废会话并回 Idle；再次明确按 OK 才重新录音。不做声学打断或 AEC。
- 按键回调只投递；网络解析和 PCM I/O 在工作任务中；LVGL 只接收有界快照。状态约 5–10Hz 更新，不随每个音频包重绘，避免挤占音频任务。
- 活动对话临时阻止熄屏，结束/取消/错误/退出后解除。Idle / Offline / Error 保留既有 60 秒熄屏和唤醒卡；退出必须先取消任务、清文本并关闭 Talk 连接，不能修改保存的游戏进度。
- 所有队列事件带连接代次和 turn ID。取消、离开、断网立即作废。当前模型验证 turn 过滤；连接代次由待实现的传输适配器过滤。
- 当前底层 I/O 可等约 1 秒，退出还受调度影响。初始可设 1.5 秒有界等待并实测；超时则禁止新任务复用资源。若要更快的硬件打断，需单独增加和测试 BSP 超时接口，不修改托管依赖。

## 4. 六种状态与按键

| 状态 | 屏幕 | 短按 OK | 麦克风 / 扬声器 |
| --- | --- | --- | --- |
| Idle | 原版 Zumi、准备提示或本轮回答 | 开始新一轮 | 关 / 关 |
| Listening | 原版专注表情、红色 REC、00–15秒 | 结束并提交 | 开 / 关 |
| Processing | 已识别文字、等待提示 | 取消 | 关 / 关 |
| Speaking | 回答文字、播放提示 | 打断，返回 Idle | 关 / 开 |
| Offline | 未连接，重连提示 | 请求连接，10秒期限 | 关 / 关 |
| Error | 超时/服务/缓冲错误 | 清错误回 Idle，不自动录音 | 关 / 关 |

UP / DOWN 翻阅文字。长按 OK 500ms 保留返回首页动作，消费释放事件，不能顺便开始录音。不依赖触摸屏。
录音最长 15 秒自动提交；Processing 最长 20 秒，识别片段不延长期限；Speaking 最长 60 秒，音频断流超过 5 秒中止，收到包不能无限延长总时间。短暂抖动缺帧填静音。
重连不自动重发录音。鉴权失败进 Error，网络断开进 Offline。后续连接自动重试最多三次，按 1/2/4秒退避后等待按键；本次预览模拟手动重连。

不重画 Zumi，复用现有男孩形象；屏幕只显示 Talk 和状态，不重复 Zumi Pocket 品牌标题。
当前设备英文标签兼容原字体。中文识别/回答需要有授权的 CJK 字体、明确覆盖范围、正确 LVGL 字体选择、有界 UTF-8 排版和实机验收。下文为其预留 Flash，但未加入字体，浏览器中文不能充当硬件验证。

## 5. 小缓冲与内存预算

以下为**规划估算，不是设备实测**。LVGL 静态池和绘制列表已经计入链接占用，不重复扣除；LCD/I2S DMA 和动态任务还会占用运行时内存。96KB 整段录音 demo 没有链接到 Zumi，也不纳入 Talk。

| Talk 新增项目 | 字节预算 | 上限 |
| --- | ---: | --- |
| 上行槽 | 3,200 | 5×640，最多排队 100ms PCM |
| 下行抖动槽 | 7,680 | 12×640，240ms；积累 3 帧/60ms 开始播放 |
| 采集/播放工作块 | 1,280 | 两块各 640 |
| 控制器与文本快照 | 2,588 | 实测 `sizeof(t_model)`；含 ASR 1,024、回答 1,536 |
| JSON RX/TX | 2,560 | 2,048 / 512 |
| 解析器固定工作区 | 4,096 | 超长消息在解析前拒绝 |
| 队列描述和事件 | 1,024 | 固定数量，不累计长字符串 |
| 新增任务栈 | 10,240 | 初始 4KiB + 6KiB，待测栈余量 |
| **应用小计** | **32,668（约31.90KiB）** | 仅控制器已实现，其余为待实现预算 |
| Wi-Fi / lwIP 增量 | 45–65KiB | 取决于连接参数、缓冲和生命周期 |
| TLS / WSS 增量 | 35–60KiB | 含握手临时内存，受证书链和配置影响 |
| **新增峰值规划范围** | **约112–157KiB** | 还需分配器和驱动余量，不保证装得下 |

高值可能超过当前显示/音频初始化后的真实余量。**RAM 是主要约束，Flash 暂时不是。** 固定音频工作区在握手前分配，慢网络不能靠无界扩容掩盖；资源不足就报错，离线游戏继续可用。

暂定验收目标：稳定 Talk 时内部 8位 Heap ≥32KiB、最大连续块 ≥16KiB；TLS 握手前最大连续块 ≥40KiB；每个新增任务至少 1KiB 栈余量。它们是待实测校准的工程门槛，不是已证实的 IDF 必需值。
失败时先基于测量优化网络缓冲/任务数量，再评估伴侣端；不默认增大 LVGL 池，不通过关闭 TLS 校验来省内存。

Flash 预留：网络/TLS/协议/UI 新增 0.6–1.2MiB；若选中文，再为有授权的 CJK 字体预留 0.4–1.2MiB，均未编译验证。
以当前应用估算新增后仍剩约 3.62–5.02MiB；以实际链接为准。不加整屏图、视频、唤醒模型、对话日志或录音文件分区，V1 不迁移 OTA 分区。

### 硬件测量方法（未执行）

采样点：启动、首页、带音效的游戏、Wi-Fi 初始化/关联、TLS 前后、Listening / Processing / Speaking、打断与清理后。

```c
const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
heap_caps_get_free_size(caps);
heap_caps_get_largest_free_block(caps);
heap_caps_get_minimum_free_size(caps);
```

另测 DMA 可用块、各任务栈最低余量、LVGL 池、队列最高水位、丢帧与供音间隔。
DMA 和内部 Heap 能力集合重叠，不能把它们相加。TLS/流式阶段需定时采样，单看前后快照会漏掉峰值。
栈水位单位依当前 ESP-IDF FreeRTOS API 文档核实。日志只含计数与错误码，不含凭据、转写、PCM 或用户身份。
重复 100 次开始/取消/断网，再连续 30 分钟语音与游戏切换，检查 Heap/连续块持续下降、任务泄漏和音频断供。

## 6. 设备与中继协议草案

传输接口：`connect`、`begin(turn, capability)`、`send_audio(frame)`、`commit(last_sequence)`、`cancel(turn)`、`close`。
中继端分别替换 ASR、单轮问答流和 TTS。服务商 Key 只在服务端通过 Git 之外的配置注入。
设备采用校验主机名/证书的 TLS，运行时配对并下发短期设备凭据，不把服务商 Key 或私人端点写进镜像。
安全配网、设备凭据刷新与时间/证书引导仍待实现；不写 API Key 不代表开放无认证中继。

控制消息为 UTF-8 JSON，接收上限 2,048、发送上限 512 字节：

```json
{"v":1,"type":"hello","capabilities":["qa"],"audio":{"codec":"pcm_s16le","rate":16000,"channels":1,"frame_ms":20}}
{"v":1,"type":"input.begin","turn":42,"capability":"qa","memory":"none"}
{"v":1,"type":"input.commit","turn":42,"last_seq":749}
{"v":1,"type":"asr.final","turn":42,"text":"What is solar power?"}
{"v":1,"type":"answer.snapshot","turn":42,"seq":0,"text":"Energy from sunlight."}
{"v":1,"type":"output.end","turn":42,"last_seq":99}
{"v":1,"type":"cancel","turn":42}
```

二进制帧头 14 字节：版本 u8=1、类型 u8（1上行/2下行）、flags u16LE=0、turn u32LE、序号 u32LE、载荷长度 u16LE，随后 1–640 字节偶数长度 PCM。
每方向序号从 0 开始。逐字段解析，不直接强转 C 结构体。验证头、精确长度、版本、flags、协商格式及当前连接代次/turn 后入队。
WebSocket 分片组装限制为二进制 654 字节、文本 2,048 字节；超长、非法 UTF-8、未知格式和畸形 JSON 在有界解析中拒绝，不按外部长度无界分配。
`answer.snapshot` 替换有限滚动窗口，不累积历史；乱序/缺序使本轮失败，已接收的本轮重复包可忽略。

录音结束先停采集，最多 500ms 排空上行，确认连续序号至 `last_seq` 后只提交一次。队列满/网络停滞中止并报错，不默默丢字再提交。
下行按空闲帧额度流控，中继不得超额；溢出报错不扩容。
`output.end` 只是服务器发送结束，本地播放队列排空后适配器才发模型 DONE；缺帧不能伪装播完。
本阶段测试的是控制状态，帧格式解析、队列与排空协议尚待实现与测试。

默认单轮问答。新一轮、离开、取消或断网清设备音频/文本，不写 NVS/Flash 历史，不启用唤醒词或持续采集。
中继会话仅临时存在，提交完成/取消/断链清理，孤儿会话 TTL 暂定 60秒；选定真实服务后另核实服务商保存策略，不能提前承诺服务商不留存。
预留 `sound_guess`、`story`、`garden_control`，V1 只接受 `qa`，拒绝未支持能力，不执行远程工具或 Garden 指令；未来 Garden 控制需明确本地确认和命令白名单。

## 7. 后续实施与验收

1. 审核交互语义、中文显示范围和中继部署方式。
2. 增加实机 Heap/栈/DMA 采样、音频所有权确认和有界传输，先用本地假服务，不接收费服务。
3. 实现 STA/配网与 PCM 适配，先测握手和稳定流，再决定是否压缩。
4. 覆盖畸形帧、超长 JSON、鉴权拒绝、无响应、慢网络、缺序、溢出、服务器结束但本地未播完、取消后迟到消息；确认 Listening 外不采集/上传。
5. 回归 4×4 游戏、音效、唤醒后原进度及 60秒熄屏；保留所有 Zumi 素材、BSP 引脚和分区。
6. 实机刷写和收费服务接入仍需分别明确确认，本次设计审核不自动授权两者。
