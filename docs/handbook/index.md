# YOYO工位绿植搭子 · RoboPlanter

> 你先下班，我负责发芽。

**把工位里的疲惫，变成一次照料。**

YOYO工位绿植搭子是一件为打工人设计的桌面互动作品：一盆真实的植物、一只屏幕里的像素精灵，以及一张可以带走的工位日签。你可以和荣耀 Robot Phone 上的 YOYO 聊聊今天，也可以看一眼植物读数、照顾小精灵，给忙碌的一天留一个停顿。

它把 **自然对话、真实传感、像素养成和热敏打印** 连成一个体验：植物需要照顾，照顾它的那个人也一样。

![YOYO工位绿植搭子装配示意：绿植、Indicator、展开云台的荣耀手机和小票出口共用一个方块外壳](/roboplanter/assets/assembly-hero.png)

*基于 Body8、Body9、Body16 的 CAD 装配展示，非成品实拍。手机正面与展开云台图像取自荣耀官方素材；[查看来源](/roboplanter/assets/assembly-hero-sources.json)。*

## 先看懂这个作品

早上到工位，说一句“早上好，上班打卡”；午后问“现在土壤湿度多少”；开了一天会，也可以只说“心累，想摸会鱼”。小芽用简短的回应接住你的话，按需查询设备，或在开启场景打印时生成一张日签。日签上有植物 Logo、有效的植物状态、三行手写待办，以及完整 78 张塔罗中随机抽取的一张牌和正逆位。

Indicator 让陪伴留在桌面上：从种子开始养一只像素植物，查看状态、切换主题、开始番茄钟，或者预览并打印日签。游戏照料和真实水泵操作分开；真实植物按照自己的节奏生长。

**想先体验：**[中文互动首页](https://peterpanstechland.github.io/roboplanter/) · [完整项目故事](product-story.md) · [工位日签设计](daily-ticket.md)

## 手机、屏幕和主控怎样配合

| 角色 | 在作品里负责什么 |
| --- | --- |
| **荣耀 Robot Phone / YOYO** | 对话入口；Skill 将明确的意图转成 BLE 请求，并根据主控的真实回包回答。 |
| **XIAO ESP32-S3 Plus + Robot Board** | 协调传感器、校准、灯带、计时和动作结果；直接通过蓝牙连接打印机。 |
| **SenseCAP Indicator D1** | 常驻触摸界面；通过 Wi-Fi/HTTP 与 XIAO 同步，提供养成、预览、设置和连接恢复入口。 |

手机直接与 **XIAO** 蓝牙配对，Indicator 显示配对码和链路状态。土壤探头接在 Indicator 内部的 RP2040 上，读数经屏幕上传给 XIAO；手机和屏幕最终使用同一份校准结果。本地交互链路无需依赖云服务器。

**想自己做：**[软硬件 BOM](hardware-catalog.md) → [制作与烧录教程](hardware-quickstart.md) → [最新接线](../wiring.md) → [土壤与水箱校准](sensor-calibration.md)

**想接入 YOYO：**[Skill 搭建](honor-platform.md) → [首次连接与日常话术](yoyo-companion.md) → [三端工程 API](system-api.md)

## 目前做到哪一步

截至 **2026-11-04 的实机记录**，已确认手机返回校准土壤百分比、保留绑定重连后正常读数、30 颗灯珠绿→白→绿、D7 超声波随目标移动变化，以及 Indicator 预览与蓝牙打印内容一致、比例正常并自动切纸。此前也已确认一次 0.5 秒短出水后停泵。

场景话术、自动打印和共享番茄钟已有实现；**完整的手机场景打印、语音计时、蜂鸣器听音及两板实际换网流程仍需逐项验收**。当前 Skill 未开放语音启动浇水。详见[功能状态与实机依据](roadmap-evidence.md)。本文于 2026-11-13 整理，不代表当天新增硬件测试。

## 资料与出处

- **项目与源码：**[公开 GitHub 仓库](https://github.com/peterpanstechland/RoboPlanter) · [Hackster 英文项目](https://www.hackster.io/peterpanstechland/roboplanter-a-talking-desk-garden-with-yoyo-146720) · [English](https://peterpanstechland.github.io/roboplanter/en/)
- **直接下载：**[固件与教程源码包](/roboplanter/downloads/roboplanter-starter.zip) · [YOYO Skill](/roboplanter/downloads/roboplanter-honor-skill.zip) · [三件 3MF 外壳](/roboplanter/downloads/roboplanter-3d-print.zip) · [文件校验清单](/roboplanter/downloads/manifest.json)
- **继续制作：**[灯带](ws2812-lighting.md) · [蓝牙打印](uart-printer.md) · [植物养成](plant-companion.md) · [安装检查](acceptance.md) · [故障恢复](troubleshooting.md)

**作者：潘石｜彼得潘的科技谷。** 外壳借鉴 Creeper 的方块语言，本项目为独立 DIY 作品；屏幕角色的生命值、心情与塔罗属于互动表达。
