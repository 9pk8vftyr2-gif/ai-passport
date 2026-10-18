[English](zumi-talk-integration.md)

# Zumi Talk：本地接入与服务身份

电脑端官方服务扩展：[小智联调与设置](zumi-talk-xiaozhi.zh_CN.md)。下文的固件及默认预览仍使用本地适配器。

2026-10-18。本次是在用户认可的 Talk 设计上接入现有主程序，替代
[原始审计](zumi-talk-v1.zh_CN.md)中的“仅设计阶段”状态。旧审计及哈希
作为历史证据保留，测试对照其固定 Git 版本，而非已经修改的当前代码。

## 已实现的交互

首页 Talk 进入共享的六状态控制器。原版 Zumi、六个 4×4 Energy Link
关卡、游戏音效、八张宣传卡和三秒后恢复游戏进度均保留。未修改网站仓库。

- 进入 Talk 自动连接，麦克风保持关闭；连接成功进入 Idle。
- OK 开始录音，再按 OK 结束并提交；连接失败后 OK 重试。UP/DOWN 翻阅介绍或文字。
- Processing / Speaking 按 OK 取消或打断，长按 OK 返回首页。
- 录音上限 15 秒、等待回答 20 秒、播放 60 秒、音频中断 5 秒、连接 10 秒。
  错误后重试不会重发旧录音。
- 录音、处理、播放及连接时保持亮屏；Idle / Offline / Error 无操作 60 秒
  熄屏。退出 Talk 或熄屏都会停止会话。
- 游戏唤醒仍显示卡片三秒并恢复原进度；其他页面唤醒回首页。
  唤醒键不会触发录音。
- 不持续监听、不保存对话、不自动开始下一轮录音。

## 实际连接范围

主预览使用真实的本地 WebSocket 连接和 Web Audio 播放。输入是**合成 PCM
测试信号**，不访问浏览器麦克风。识别文字和回答为固定样例，不能视为真实
语音识别或 AI 推理。预览测试区域可选择“认识 Zumi”或“太阳能”。
测试语音由本机系统生成，不是已经选定的正式 Zumi 音色。

固件已链接 Wi-Fi STA / WebSocket 适配器和原 BSP ES8311 / I2S，但未在
设备上运行。适配器读取已有 NVS `zumi_talk` 中的 `ssid`、`password`、
私网 IPv4 `host` 和可选 uint16 `port`（默认 8881）。本次**没有配网界面
或凭据写入工具**，缺少配置时保持 Offline。未擦除、重置、刷写或记录密码，
实体设备也未连接服务商。

本地适配器仅接受私网 IPv4 和普通 WS，没有生产级认证或 TLS，不是公网服务。
模拟服务默认只监听本机回环地址；可选 LAN 监听仅用于后续获准的可信测试网络
真机测试。浏览器里的 localhost 不是设备可以使用的电脑地址。

## 本地预览

在仓库根目录，用不同终端运行服务：

```sh
python3 -m http.server 8876 --bind 127.0.0.1
python3 -m venv build/talk-venv
build/talk-venv/bin/pip install -r tools/talk_mock_requirements.txt
python3 tools/build_talk_mock_audio.py
build/talk-venv/bin/python tools/talk_mock_server.py
```

音频生成需要 macOS `say` 的 Daniel 男声（每分钟 180 词，短句间轻微停顿）和 `ffmpeg`。
也可按照 `tools/talk_mock_fixtures.json` 的文本自行提供单声道、16kHz、
PCM16 WAV：`build/talk-mock/intro.wav` 与 `solar.wav`。系统语音输出仅保留
在被 Git 忽略的 `build/` 中，不分发、不嵌入固件。打开
<http://127.0.0.1:8876/preview/>。原 `/preview/talk/` 仅保留为视觉设计参考。

故障模式：`--fault error`、`disconnect`、`timeout`、`bad-frame`；
可用 `--port` 与预览 `?talk_port=` 指定其他本地端口。
服务只保留有界连接/轮次计数，不落盘录音，闲置连接 60 秒后关闭。

## Zumi 身份与小智核心

**面向用户的是 Zumi，小智是已选择的电脑端语音后端。** 固件及默认预览仍连接
本地模拟服务。[config/zumi-persona.json](../../config/zumi-persona.json)
独立于服务商、不含凭据；中文角色及经核对的网站知识现已应用到官方电脑测试智能体，
详见[角色验收报告](zumi-talk-persona.zh_CN.md)。

- Zumi 是 ZCO 的绿色生活伙伴。Talk 不显示网址标记、不强调性别。
  说话温暖、好奇、简短，用生活例子解释绿色知识，也回答普通问题。
- Zumi Pocket 是随身互动产品，`zco.eco` 是 ZCO 网站；网站提供角色、社区、
  游戏和工具背景。本版没有网站账号同步，也不能控制 Garden。
- 网站知识与角色身份分开维护，采用注明日期的资料快照。提示词里写网址
  不等于模型了解或访问过网站。最初核对本地源码，随后已在云端角色验收中
  核对实时 Zumi 和 About 页面；
  未知或更新的功能须等待核实，不能编造。
- 用户询问底层技术时如实说明实际服务。不默认长期记忆；托管平台的数据
  留存规则需要另行核对；本次已在官方控制台保存并回读确认 Memory 关闭。
- 验收提问覆盖身份、ZCO、网站关系、实际服务、账号访问、Garden 控制以及
  游戏分数是否代表真实减碳。固定样例只验证表达与播放，不能证明 LLM 遵守角色。

[小智 WebSocket 协议](https://github.com/FoloToy/folo-ai-passport-xiaozhi/blob/72da544d1b51678f5d88967adc299b3aec956946/docs/websocket.md)
包含设备/客户端身份、认证头、hello/session、Opus 音频、手动 listen
开始/结束、abort、STT 与 TTS 消息。现有本地 PCM 协议**不能只换 URL 就直连**。
需要独立适配器、Opus 编解码、协商采样率处理、TLS、设备激活和运行内存验证。
不复制整套小智固件，不替换 Zumi 界面或分区。应用继续使用 ESP-IDF 5.5.3，
引用的小智移植仓库有不同 SDK 要求。控制台角色/音色选项、设备绑定、知识接入、
留存与服务条款，须在选定账号中核实。电脑端已使用用户确认可用的官方免费服务，没有选择收费服务。
若实测 C3 内存无法同时容纳 Opus 和 TLS，可采用受控中继作为备选。

## 所有权、协议与内存

`core.c` 管理导航和 Talk 状态，`view.c` 绘制共享 UI；设备工作队列不操作
LVGL。`talk_device.c` 独占 STA、事件与连接，`audio.c` 独占 Talk 和游戏的
所有 BSP 音频 I/O。停止先作废轮次/代次再排空队列；音频停止确认意味着工作
任务已经返回当前 BSP 调用并应用停止版本。提交等待此确认，再发送末帧编号。
停止时尚未完成的采集块丢弃。现有 BSP I/O 超时可达一秒，实际打断时延未测。
播放完成等待应用队列排空，再增加 100ms DMA 尾部保护；仍须真机验证。

本地协议为 `hello/ready`、`input.begin`、顺序音频块、`input.commit`、
`asr.final`、`answer.snapshot`、回复音频块与 `output.end`。
录音中分块上传，提交才表示用户说完并请求回答。音频为 16kHz 单声道
PCM16，每块 20ms / 640 字节，另加 14 字节小端版本/类型/标志/轮次/序号/长度头。
播放端额度最多允许 12 个未消费应用帧，以消费额度调节传输，并在浏览器预留
120ms 播放缓冲，避免逐帧固定等待累计造成断续。设备音频活动通知合并为
约 200ms 一次，避免突发流量占满 UI 队列。

旧轮次丢弃；错误序号、格式、长度或队列溢出停止会话。cJSON 解析前限制
JSON 2048 字节、深度 4、结构/字符串 token 48。本版样例与显示约定是英文
ASCII，中文字体仍待接入。TCP 分段在一秒内组装，拒绝 WebSocket 分片消息；
取消时如有不完整负载则关闭连接。网络调用不在按键回调里执行。

| 分配项 | 预算 / 证据 |
| --- | --- |
| 应用状态 | 每份 2700 字节，两份静态快照；Talk 模型 2588 字节 |
| 现有绘制列表 / LVGL 池 | 2×6404 / 24576 字节，已计入链接静态占用 |
| 现有 LCD 缓冲 | 运行时分配 19200 字节，不额外分配全屏缓冲 |
| PCM 队列 | 5×648 + 12×648 = 11016 字节，另加 FreeRTOS 队列元数据 |
| UI 通知队列 | 4×1548 = 6192 字节，另加元数据 |
| 命令队列 | 8×12 = 96 字节，另加元数据 |
| 任务栈 | 网络 6144，原音频 4096，主任务 6144（增加 2048）字节 |
| 静态网络暂存 | 2×2049 JSON、654 协议帧、2×1548 通知；计入链接静态占用 |
| Wi-Fi 缓冲 | 静态 RX 6、动态 RX 8、动态 TX 8，限制无 PSRAM 场景峰值 |
| 其余运行时成本 | Wi-Fi/LwIP/任务栈、I2S DMA 与驱动、cJSON、socket 缓冲和分配开销；须测量 |
| TLS / Opus | 未实现，未计入已测运行数字 |

链接后的 RAM 余量不等于可用 Heap。已预留 before-connect、connected、
listening、speaking、completed 阶段的内部 free/largest/minimum heap 日志。
接真实后端前须记录游戏独立运行、Wi-Fi 建连、重复对话、断网与退出后的数值。
缓冲和任务队列跨轮次复用，不按整段录音分配内存。

## 验证与硬件验收

[integration.json](talk-v1/integration.json) 记录实际固件、源码哈希、分区、链接内存与测试状态，
只依据完成的检查更新。历史审计单独保留。

| 字段 | 范围 |
| --- | --- |
| Build | 官方完整 ESP-IDF 5.5.3 门禁、合并镜像及配套 ELF 校验 |
| Host Tests | core / wire / state / 音频所有权测试、取消与队列故障注入、native/WASM 对照、七项真实本地 WebSocket 测试 |
| Browser Demo | 主预览、语音样例、打断、后台关闭/重连、服务错误/断网/超时/坏帧恢复、游戏与全部唤醒卡 |
| Device Tests | NOT RUN；未访问串口、刷写、擦除或重置 |
| Unverified | 真实麦克风/扬声器/Wi-Fi、Heap 三指标、取消与 DMA 排空时延、功耗/耐久、配网、设备 TLS/认证/Opus、中文显示、正式音色与模型角色遵循 |

硬件到货且获准测试后：检查按键与录音状态指示、每阶段录音/播放/取消、
游戏与 Talk 的音频所有权；错误密码、AP 不可达、断网重连；100 轮对话的
Heap 与任务栈峰值；一小时离线游戏/唤醒回归；熄屏、Wi-Fi 闲置、录音和
播放的整机电流。真实服务验收还须检查绑定、角色/知识问答、关闭自动监听、
留存设置，以及凭据处理不进入代码仓库。

## 本轮结果

Build **PASS**；Host Tests **PASS**；Browser Demo **PASS**；Device Tests **NOT RUN**。

应用 2,742,464 字节，factory 剩余 5,580,608 字节；链接 DIRAM 余量 138,722 字节，**不是运行时可用 Heap**。完整镜像 SHA-256：`c6b3dc0d19adf0c809912723d7474e8d2ba0fc6c772d1acbbb23b28a61705117`；配套 ELF：`c5f54f2ca1e4065172a90aba90dfc4987367c5b8be08c72292366f16eeb09da9`。

浏览器本轮包含完整播放及两次取消：接收 490 帧、播放 466 帧、最大排队 12 帧、断流计数 0；差额 24 帧为取消时丢弃的已排队帧，完整播放轮次另断言全部播放。
