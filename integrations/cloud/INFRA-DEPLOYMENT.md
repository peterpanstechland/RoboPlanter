# RoboPlanter 云端基础设施变更记录

更新：2026-10-24（Asia/Shanghai）。本文不包含密码、Token 或私钥。

## 部署位置与边界

- 跳板：`ubuntu@192.168.66.22`，基础设施目录 `/home/ubuntu/ansible-infra`。
- 目标：inventory 的 `aliyun-gdd`，`root@8.129.178.255`。SSH 使用跳板已有密钥，全程 `BatchMode=yes`、`StrictHostKeyChecking=yes`。
- 新服务目录：`/opt/roboplanter`，目录权限 `0700`。
- 已上线 HTTPS 路径：`https://emqx.robomon.cn/roboplanter/`。无新增域名、证书、公共监听端口或 MQTT broker。
- 应用只监听宿主机 `127.0.0.1:18765`，容器内部 `8765`。反向代理剥离 `/roboplanter/` 前缀；应用 `ROBOPLANTER_ROOT_PATH=/roboplanter`。
- MCP 路径：`/roboplanter/mcp`；兼容 SSE 路径：`/roboplanter/sse` 和 `/roboplanter/messages/`。
- 本轮云工具只允许查询。设备动作仍应由云端明确拒绝，不能把服务上线理解为远程浇水已开放。

## 已核实的既有服务

EMQX 为 `emqx/emqx:5.10.3`，容器 `emqx`，部署目录 `/opt/emqx`，网络 `emqx_default`。TLS MQTT `mqtt.robomon.cn:8883` 的系统 CA 与主机名校验成功；明文 MQTT `1883`、WebSocket `8083`、Dashboard `18083` 只绑定回环地址。本次只新增下文记录的两个用户和专属 ACL，未重启 broker 或修改监听器。

既有 Nginx 站点为 `/etc/nginx/sites-enabled/emqx-dashboard.conf`。`emqx.robomon.cn` 与 `mqtt.robomon.cn` 已指向该主机，现有证书覆盖这两个名称；未发现 RoboPlanter 子域名或可复用的通配符证书，因此选用现有域名的独立路径。

EMQX 使用用户名密码内置数据库认证，并有其他既有认证机制。实际文件授权源为 `/opt/emqx/data/authz/acl.conf`，包含允许所有的既有兜底规则。为保护其他业务，不能直接改全局兜底：新账号必须在兜底之前拥有专属 allow 规则与 deny 其他主题规则，且 `is_superuser=false`。

## 本次已完成

- 创建 `/opt/roboplanter/.env`，权限 `0600`：独立随机 MCP Bearer Token、云端专属 MQTT 账号随机密码、固定 broker/设备 ID 与 HTTP Host/Origin 白名单。
- 创建 `/opt/roboplanter/.env.gateway`，权限 `0600`：网关专属 MQTT 账号随机密码；不注入云服务容器。
- 重跑准备脚本会保留已有凭据，不自动轮换。
- 原始配置备份（权限 `0600`）：
  - `/opt/roboplanter/backups/20261023T223440Z-emqx-dashboard.conf`
  - `/opt/roboplanter/backups/20261023T223440Z-acl.conf`
- 生成并验证的 Nginx 文件（下文记录其后续激活结果）：
  - `/opt/roboplanter/nginx/emqx-dashboard.conf.proposed`
  - `/opt/roboplanter/nginx/roboplanter-location.conf`
- 新增路径使用回环 upstream、保留 Host、关闭 proxy buffering/cache、300 秒 SSE 超时，并将请求体限制为 32 KiB。现有其他路径保持原配置。
- 激活前使用独立顶层测试配置 `/opt/roboplanter/nginx/nginx-test.conf` 执行 `nginx -t` 成功（退出码 0）。输出另含其他既有站点的监听参数/MIME 警告，本次未修改这些站点。
- 两份新凭据已经由 SCP 传回本地 ignored 文件 `.env.cloud`、`.env.gateway`，本地权限限定当前用户与 SYSTEM。跳板中转文件已清理；未在消息中展示凭据值。

原始 Nginx SHA-256：`52c042bbef661da26a94280e6b7bd2df2af0642e2b4305d82ca78775177a5c1a`。
原始 ACL SHA-256：`4118a923a35404cd7cfb3746a884992fe0445a6aa3299bfde33008ead8d237d4`。

## MQTT 账号与隔离验证

| 账号 | 允许发布 | 允许订阅 |
|---|---|---|
| `roboplanter-cloud` | `roboplanter/v1/roboplanter-01/query` | `roboplanter/v1/roboplanter-01/reply`、`roboplanter/v1/roboplanter-01/presence` |
| `roboplanter-gateway` | `roboplanter/v1/roboplanter-01/reply`、`roboplanter/v1/roboplanter-01/presence` | `roboplanter/v1/roboplanter-01/query` |

以上均为精确主题，允许发布的消息必须 `retain=false`。两个账号均已通过官方管理 API 创建（HTTP 201），并重新查询确认 `is_superuser=false`。

先备份并热更新 ACL（HTTP 204），再创建账号，避免新账号短暂继承原兜底权限。六条专属规则添加在原规则前，原有规则全文保持不变。每个账号的精确 allow 后使用 `{deny, {username, "账号名"}, all, all}.`，第四项为原子 `all`，覆盖任意 `$` 前缀主题。该形式已由[运行版本 e5.10.3 官方源码](https://github.com/emqx/emqx/blob/e5.10.3/apps/emqx_auth/src/emqx_authz/emqx_authz_rule.erl#L158)核对，并完成实际测试；没有使用会漏掉部分 `$` 主题的普通 `#` 兜底。

- 更新前追加备份：`/opt/roboplanter/backups/20261023T225349Z-pre-roboplanter-acl.conf`。
- 更新后 ACL SHA-256：`cc40f249b056aab27988f95a61798f8a81b3545d38e5b14565b197c0c5295f1c`。
- 未改全局 `no_match`、其他用户、认证链、监听器或既有兜底规则；未重启 broker。
- 通过系统 CA 与主机名验证的 MQTT TLS 8883，使用两个临时测试客户端完成 **24 项实际 broker 检查**：3 项合法订阅、3 项合法发布并确认对端收到、10 项非法订阅、8 项非法发布。非法项包含反方向、跨设备、`#`、`$SYS/#`、任意其他 `$` 前缀，以及 retained 发布；拒绝的 MQTT 5 SUBACK/PUBACK 均为 135（Not authorized）。
- 测试消息为无效业务格式的短期探测标记，不是设备命令、传感器数据或有效在线声明；临时客户端已经断开。
- 最终管理 API 复查：`roboplanter-cloud` 存在 1 个已连接客户端，即运行中的云服务。EMQX 与新 MCP 容器最终均为 `running healthy`。

服务器现有初始化 Dashboard 密码登录曾返回 HTTP 401。之后用户明确提供本地 `.env.emqx-admin`，仅用该指定文件登录成功（HTTP 200），不再存在管理凭据缺口。现行用户与 ACL API 已依据运行中版本的 `/api-docs/swagger.json` 核对。未新增/重置 Dashboard 管理员，未搜索其他应用的凭据，未使用底层数据库/Erlang 操作绕过管理接口。

云端 `/opt/roboplanter/.env.emqx-admin.tmp` 与跳板的本次管理员凭据中转目录已删除；本地用户原始 `.env.emqx-admin` 保留且权限收紧。管理员凭据没有进入镜像或服务环境；云服务仅持有自身非超级用户的 MQTT 密码。跳板上的源码中转包也已删除，工作区原包与云端部署源码保留。

## 应用部署与公网验证

2026-10-24 06:46（Asia/Shanghai）已启动 `roboplanter-roboplanter-mcp-1`，Docker health 为 `healthy`。使用已验证源码包 `cloud-source.tar.gz`（31,392 字节，SHA-256 `4f08b49e1d2e94681b30ecec52e6be2286eb87e679b0578c37f2008e0a39d05e`），20 个成员经过路径与类型检查，无路径穿越、链接或实际 `.env` 文件。解压目录 `/opt/roboplanter/source`；基础镜像为 `python:3.12-slim`，首次构建成功，无失败重试。

启动命令（在云端执行，不包含凭据）：

```sh
ROBOPLANTER_ENV_FILE=/opt/roboplanter/.env docker compose -p roboplanter -f /opt/roboplanter/source/integrations/cloud/compose.yaml up -d --build
```

独立 Docker 网络 `roboplanter_default`；只暴露 `127.0.0.1:18765`，非 root 用户运行、只读根文件系统、丢弃 capabilities、禁止新增权限。限制为 192 MiB RAM、0.5 CPU、64 PIDs；一次空闲附近观测为 **44.06 MiB RAM、1.32% CPU、3 PIDs**，不是持续负载基准。自动重启策略 `unless-stopped`，日志每份 2 MiB、最多 2 份。

健康和鉴权通过后，Nginx 提案已激活，活动配置再次 `nginx -t` 成功后仅 reload。活动站点 SHA-256：`fb134e34dd2a2325d7acf2845cfd049ef84d80682fb1b42a920ec481a20dadd9`。既有 EMQX 仍为 `running healthy`，没有重启。

从本地计算机通过公网 HTTPS、标准 CA 校验完成实际 JSON-RPC 验证：

- `/roboplanter/healthz`：HTTP 200，`writes_enabled:false`。
- 不携带 Bearer 的 `/roboplanter/mcp`：HTTP 401。
- 携带 Bearer 在内存中调用 `initialize`：协议版本 `2025-11-25`；`tools/list` 返回六个既定工具。
- `get_planter_state`：`unavailable / gateway_unavailable / read_only:true`。
- `water`、`stop_water`、`set_light`、`print_note`：全部 `rejected / cloud_writes_not_enabled / sent:false`，未发送硬件命令。
- `get_action_result`：`unknown / gateway_unavailable`，不被误报为未执行。

服务端使用官方 MCP Python SDK 1.30.0。公网验证的客户端为 Python 标准库 JSON-RPC；另外在容器内用官方 SDK 1.30.0 实际连接运行中的 HTTP 服务，初始化与六工具发现均通过（协议 `2025-11-25`）。两类验证分别记录，不将容器内客户端等同于荣耀平台或真实硬件验证。

## Ansible 持久化检查与修复

2026-10-24 07:24（Asia/Shanghai）检查确认：跳板 `/home/ubuntu/ansible-infra/playbooks/emqx/deploy.yml` 的 Nginx template 任务会整体重写 `/etc/nginx/sites-enabled/emqx-dashboard.conf`，原模板没有 RoboPlanter 路由。因此已在该基础设施仓库做以下最小变更：

- `playbooks/emqx/vars/roboplanter.yml`：非敏感变量，限定目标主机列表为 `aliyun-gdd`，固定路径 `/roboplanter` 与回环 upstream。
- `playbooks/emqx/templates/roboplanter-nginx-location.conf.j2`：独立路由模板。
- `playbooks/emqx/templates/emqx-nginx.conf.j2`：仅对目标列表中的主机条件 include 新模板。
- `playbooks/emqx/deploy.yml`：加载新变量文件，并为原有 Nginx template 任务增加 `emqx-nginx` 标签，以便单独审查该任务。
- `playbooks/emqx/ROBOPLANTER.md`：维护说明、语法检查与 `--check --diff` 审查命令。

两个被编辑的原文件备份在跳板 `/home/ubuntu/roboplanter-infra-backup-20261023T232354Z/`。没有运行部署 playbook、提交 Git、改动其他已有工作区变更或 reload 服务。改动目前留在该仓库工作区；后续清理工作区或切换分支时应保留这些文件。

验证均通过：Jinja 严格变量渲染（目标主机有路由、无关主机没有路由）、Ansible `--syntax-check`、`git diff --check`；生成的完整站点通过云端独立测试配置 `/opt/roboplanter/nginx/nginx-ansible-test.conf` 的 `nginx -t`，活动配置哈希保持不变。

现行 EMQX playbook 没有写入 `data/authz/acl.conf` 的任务，Compose 的 `/opt/emqx/data` 绑定保留账号与 ACL，因此普通重跑不会覆盖本次 ACL。删除或重建整个数据目录仍会丢失这些状态，应依既有备份流程保护该目录，不要用管理员密码重置来恢复业务账号。

## 仍未完成的硬件与平台验证

- 真实本地网关与 XIAO 尚未接入此次云端部署；当前应如实返回离线/不可用，不能虚构传感器数据。
- MCP SDK/mock 测试、HTTPS 服务可用、荣耀平台成功导入工具、真实硬件查询成功是不同验证阶段；只有实际完成的阶段才能报告成功。
- 云服务健康检查 Host 白名单须包含 `127.0.0.1:8765`；对外允许的 Origin 为 `https://emqx.robomon.cn`。

官方参考：[EMQX REST API](https://docs.emqx.com/en/emqx/latest/guides/api.html)、[内置数据库认证](https://docs.emqx.com/en/emqx/latest/guides/access-control/authn/authn.html)、[文件授权](https://docs.emqx.com/zh/emqx/latest/guides/access-control/authz/file.html)。实际变更应以运行中 5.10.3 的 API schema 与 CLI 为准。
