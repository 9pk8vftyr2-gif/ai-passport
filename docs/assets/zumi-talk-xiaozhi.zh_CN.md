[English](zumi-talk-xiaozhi.md)

# Zumi Talk：官方小智电脑联调

2026-10-18。本次扩展[本地接入](zumi-talk-integration.zh_CN.md)。用户已在官方控制台
绑定独立的电脑测试客户端，并确认有免费服务。已通过严格 TLS 校验的 WebSocket
握手和一次完整合成语音往返。这是**电脑端**结果；ESP32-C3 固件目前仍使用本地
PCM 适配器，尚未直接接入云端 Opus/TLS。

## 开始对话

打开 <http://127.0.0.1:8876/preview/?talk=xiaozhi>。选择 Talk 后自动连接，麦克风
保持关闭。按 OK、允许麦克风权限、说话，再按 OK 结束录音并等待回答。回答时按
OK 打断，长按返回首页。每次最多录音 15 秒。离开或隐藏页面会停止采集/播放并关闭
本地会话。录音过程中会流式发送声音，并非先保存整段再上传；手动模式下结束消息
通知服务端开始回答。

使用[角色设置页](../../preview/talk/setup.html)复制准备好的 Zumi 设定，在已绑定
设备对应的官方角色设置中粘贴；选择当前可用的免费男声，关闭长期记忆，保存后退出
Talk 再进入。这些设置**已于 2026-10-18 保存并重新打开核验**：自定义 Zumi 角色、阳光青年男声、
标准语速/音高、Memory 关闭，详见[角色验收报告](zumi-talk-persona.zh_CN.md)。没有通过
未公开的协议注入角色或音色。知道网站网址不等于能实时浏览网站或访问账号。

普通地址 <http://127.0.0.1:8876/preview/> 继续使用本地合成输入与模拟服务。官方
模式需要明确切换，使用独立的本机 8882 桥接服务。原版形象、离线游戏和唤醒逻辑保留。

## 启动电脑服务

使用 Python 3.9+ 和现有锁定版本的 WebSocket 依赖：

```sh
python3 -m venv build/talk-venv
build/talk-venv/bin/pip install -r tools/talk_mock_requirements.txt
python3 -m http.server 8876 --bind 127.0.0.1
```

另开终端，通过本机包管理器安装 libopus，或把 `ZUMI_OPUS_LIBRARY` 指向已有的可信
libopus 动态库。项目不重新分发该二进制。随后运行：

```sh
python3 tools/xiaozhi_setup.py request-binding
# 在官方控制台绑定显示的号码，再刷新元数据：
python3 tools/xiaozhi_setup.py request-binding
build/talk-venv/bin/python tools/talk_xiaozhi_relay.py --probe
build/talk-venv/bin/python tools/talk_xiaozhi_relay.py
```

会复用同一个私有电脑身份。已绑定无需重新申请号码；`status` 只读本地状态、不联网。
元数据请求忽略固件更新；probe 仅做握手，不发送音频或调用推理。真正录音由上面的
浏览器按键动作触发。

凭据和软件身份放在用户私有应用支持目录中，**同时位于 Git 和 HTTP 服务根目录之外**，
目录权限 0700、文件权限 0600。不得把 Token 放入聊天、网址、源码或构建日志。
桥接仅接受指定本地浏览器来源，同一时间一个客户端，仅监听 127.0.0.1，不开放局域网。

## 协议与缓冲边界

浏览器 AudioWorklet 将单声道麦克风输入转为 16 kHz PCM16，每 20 ms 发出 320 个
采样点。采集使用独立 AudioContext，停止时关闭；迟到的权限授权也会立即停止音轨。
打开页面和进入 Talk 都不会自动申请麦克风。

桥接将三个 PCM 块合为一个 60 ms Opus 包，检查轮次、顺序和长度，末尾最多补齐一个包。
严格保留 TLS 证书与主机名校验。实现 WebSocket v1、hello、手动 listen start/stop、
abort、STT、TTS 句子文本和原始 Opus 音频，依据
[上游协议](https://github.com/FoloToy/folo-ai-passport-xiaozhi/blob/72da544d1b51678f5d88967adc299b3aec956946/docs/websocket.md)。
不启用 MCP、摄像头、电脑工具、远程重启或固件命令。

实测服务端协商为 24 kHz 单声道 Opus；libopus 直接解码到现有播放路径的 16 kHz。
电脑浏览器协商最多 36 个 PCM 块（720 ms），起播预留 300 ms，实际断流后重新预留
200 ms；通过播放额度控制流速。旧客户端与本地模拟仍使用 12 帧窗口。
解码暂存最多 120 ms PCM，上行累积最多 60 ms；WebSocket 接收队列最多四条消息，
云端 JSON 上限 64 KiB。这些是电脑端边界，不是已经验证的 C3 内存预算。

正常完成后现会复用当前 Talk 会话，支持连续追问；取消或出错后关闭会话，恢复时新建，
隔离迟到音频。退出、断开或空闲超时也结束会话，不保存本地长期对话历史。
详见[识别与恢复报告](zumi-talk-recognition.zh_CN.md)。
文本有长度上限，按 UTF-8 安全截断，并通过 `textContent` 显示。中文回答显示在浏览器
文字区；硬件共享屏幕暂显示简短英文提示，因为设备中文字库尚未完成。

运行时不写入录音或转录文本，也没有实现本地长期记忆。关闭会话不能证明官方平台已经
删除数据或关闭留存，需在账号内另行核实。

## 验证

| 分类 | 结果 |
| --- | --- |
| Build | PASS：现有 IDF 5.5.3 本地适配器固件；电脑桥接没有改动固件 |
| Host Tests | PASS：绑定及凭据保护、真实 libopus 往返、离线桥接完整流程、额度/序号边界、取消、新会话、云端错误和 UTF-8 截断 |
| Browser Demo | PASS：官方安全握手，公开合成问候被识别、收到回答文本，123/123 音频块播放，最大队列 12、观测断流 0；自动测试未使用真实麦克风 |
| 采集生命周期 | PASS：浏览器模拟媒体验证按键前不开麦、权限拒绝、失焦取消、迟到授权后关闭音轨 |
| Device Tests | NOT RUN；没有刷写、擦除、重置或串口操作 |
| Unverified | 真人麦克风与声学、主观音质、云端留存、长期网络恢复、C3 Opus/TLS/Heap、设备中文字库 |

官方测试使用公开合成句子“Hello. Please say hello in one short sentence.”。
[测试记录](talk-v1/xiaozhi-desktop.json)只保存非敏感计数。联网浏览器测试需要显式设置
`ZUMI_TEST_OFFICIAL=1`，不得自动加入 CI 或固件检查。离线协议测试运行：

```sh
python3 tests/test_xiaozhi_setup.py
build/talk-venv/bin/python tests/test_xiaozhi_relay.py
build/talk-venv/bin/python tests/test_talk_opus.py
```

硬件验收前：完成控制台角色/音色/记忆检查；测量 Wi-Fi、音频、显示同时工作时的可用
Heap 与最大连续块；实现并核算设备云端适配器或可信桥接；确认中文字形覆盖；最后单独
授权实体测试。电脑端联调通过不等于授权刷写。

登录自动启动、前台自动重连及本次播放和嘴型测试见[电脑端稳定性](zumi-talk-desktop-reliability.zh_CN.md)。
