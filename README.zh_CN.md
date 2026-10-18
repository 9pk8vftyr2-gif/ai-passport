[English](README.md)

# Zumi Pocket

面向 FoloToy AI Passport 的离线三键口袋伙伴：ESP32-C3、240×320 竖屏、
8MB Flash、无 PSRAM。Zumi 是男孩。本原型使用 ZCO 现有米白 2D 母版，
仅制作轻量的面部表情衍生，未重新设计角色外观。

## 体验浏览器原型

在仓库根目录运行：

```sh
python3 -m http.server 8876 --bind 127.0.0.1
```

打开 <http://127.0.0.1:8876/preview/>。已提交 WebAssembly 模块，预览无需安装
依赖包。使用屏幕下方三键，或键盘 ↑ / ↓ / Enter；长按 OK / Enter 500 毫秒
返回首页。屏幕本身没有触摸操作。保持本地服务器运行，不需要外部服务。
页面本地资源加载完成后，断网也能继续游戏。

## 第一阶段内容

- **首页：** Talk / Play / Garden，以及开心、专注、欢呼三种基础表情。
- **Carbon Pop：** 上下选择三行之一，OK 消除 CO2，保护叶子和阳光。
  正确命中得 10 游戏分；误击或错过 1.8 秒目标扣一颗心，共三颗心。
  坚持 30 秒完成一局。
- **Pocket Journey：** 上下切换三条跑道，收集绿色 + 标记得 5 分，OK 跳过石块。
  跳跃持续 600 毫秒，冷却 1,000 毫秒。坚持 30 秒完成；撞石块扣一颗心。
  跳跃时免受石块伤害，仍可收集标记。
- **Talk / Garden：** 明确的未接入状态页及 C 接口，不录音、不请求网络、
  不接入付费 AI，也不执行真实 Garden 同步。
- **结算：** OK 重玩，长按 OK 回首页。分数仅用于游戏，不代表实测减排量。
  重启后清空本次状态，本阶段不保存游戏数据。

固件和浏览器编译**同一份 C 游戏逻辑与绘制指令**。浏览器使用 Canvas，
固件通过原有 BSP 使用 LVGL。浏览器是行为预览，不代表真实屏幕显示和时序验收。
原硬件测试源码保留为参考，但不链接到本应用入口。

## 开发与验证

使用本仓库 main 基线要求的 ESP-IDF **5.5.3**，不混用小智移植仓库要求。
执行 `python3 tools/install_passport_skills.py --install` 安装官方五个技能，
再执行 `--check` 检查。

```sh
. /path/to/esp-idf-v5.5.3/export.sh
idf.py --version
./tools/validate.sh
```

仅主机门禁使用 `./tools/validate.sh --static`；仅固件门禁使用
`./tools/validate.sh --firmware`。这些命令均不刷写设备。构建成功后，
`build/firmware/` 保留经核验的合并镜像及对应 ELF/MAP；仅应用镜像不是完整设备镜像。

重新编译浏览器模块需 wasi-sdk 34：

```sh
WASI_SDK_PATH=/path/to/wasi-sdk-34 ./tools/build_preview.sh
node tests/wasm_parity.cjs
```

制作及测试依赖可执行 `pnpm install --frozen-lockfile` 安装（需要 Node.js）。

一致性测试编译原生 C 回放，并与 WebAssembly 比较 5,000 次状态转换。
安装 Playwright 和 Chrome 后，保持预览服务器运行，执行
`node tests/browser_zumi.cjs`；其他 Playwright Chromium 通道可通过
`PLAYWRIGHT_CHANNEL` 指定。截图和机器可读结果保存在被忽略的 `build/`。

资源编译器为 `node tools/build_zumi_assets.cjs`，依赖 Sharp。
原始角色和来源记录位于 `assets/images/zumi-pocket/`，生成的 C 图像资源已提交，
固件构建无需 Node 或 Sharp。

详见[设计与硬件验收](docs/assets/zumi-pocket.zh_CN.md)及
[验证报告](docs/assets/zumi-pocket-validation.zh_CN.md)。

## 分支边界

Fork：`9pk8vftyr2-gif/ai-passport`；分支：`feature/zumi-pocket`。
基于上游 main `3d685a90bdb96dd8526d84edde1e63fec25b460e`。
上游和 Fork 的 main 均保持不变。本项目不修改 ZCO 网站、不部署到 Cloudflare、
不刷写设备、不擦除存储、不接入付费 AI 服务。
