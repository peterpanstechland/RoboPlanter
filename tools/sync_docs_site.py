"""Copy public RoboPlanter documentation into the explicitly selected Docusaurus checkout.

No git, deployment, credential loading, or network operations are performed.
Only the managed project directory, static/roboplanter, a bounded projects
index block and the project's sidebar translation are changed. Vendor attachments and private settings are
never copied. Run from any directory with Python 3.10+.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import zipfile
from package_public_starter import package as package_starter, source_files, write_member
from package_honor_skill import package as package_honor_skill

ROOT = Path(__file__).resolve().parents[1]
PUBLIC_BASE = "/docs/projects/2026/roboplanter"
PAGES = [
    ("index", "YOYO工位绿植搭子 · RoboPlanter", "一盆真实绿植、一只像素精灵、一张工位日签：从项目故事到制作与工程接口。"),
    ("product-story", "项目介绍", "了解工位搭子的设备分工、日常使用与产品设计。"),
    ("hardware-catalog", "硬件图鉴与产品链接", "HONOR Robot Phone、Seeed XIAO ESP32-S3 Plus 与 SenseCAP Indicator D1 的官方产品图、用途和链接。"),
    ("hardware-wiring", "当前硬件连接", "2026-11-04接线、D7超声波、SEN54分线供电、BLE打印与当前验收状态。"),
    ("sensor-calibration", "土壤与水箱校准", "Indicator设置中的干湿、满水、缺水和空箱参考点采集、保存与清除。"),
    ("yoyo-companion", "YOYO陪伴与设备控制", "Robot Phone入口、首次配对、场景日签、共享番茄钟与日常话术。"),
    ("daily-ticket", "植物塔罗与工位日签", "Logo、真实状态、手写待办、78张塔罗和场景打印设计。"),
    ("hardware-quickstart", "硬件从零到首次读取", "XIAO 与 Indicator 的认板、供电、备份、接线、构建和分阶段联调。"),
    ("plant-companion", "植物生长与陪伴养成", "真实物候、预测与陪伴反馈分开，逐步替换加速演示周期。"),
    ("crop-knowledge", "作物知识库与地区化方案", "官方农业资源、学术依据、开源工具及带适用条件的离线原型。"),
    ("ws2812-lighting", "WS2813 灯带与氛围交互", "灯带接线、四种模式、HTTP/BLE/CLI 控制与分阶段实机验收。"),
    ("uart-printer", "小票打印机：BLE与历史UART", "当前XIAO直接BLE打印；历史UART接线、中英文点阵、自动切纸和屏幕预览。"),
    ("honor-platform", "从零搭建荣耀 YOYO Skill", "图文讲解技能文件、导入配置、安全检测、手机安装和 BLE 逐层验收。"),
    ("ble-integration", "BLE 完整接入教程", "官方工具参数、设备协议、MTU、加密绑定、回包语义和逐步验收。"),
    ("system-api", "Skill 与三端工程 API", "Robot Phone、XIAO和Indicator的链路、状态归属、配对、读写接口、打印时序与恢复。"),
    ("device-platform", "自带硬件与 Device Kit", "统一能力描述、离线登记和验证，以及未来在线平台的边界。"),
    ("design-system", "产品视觉与交互设计", "工位搭子的视觉语言、角色、状态、动效和可访问性规则。"),
    ("social-playbook", "拍摄与分享", "项目图片、短视频、角色文案和接入经验的分享方法。"),
    ("roadmap-evidence", "功能进度与开发计划", "查看模块支持情况和接下来的开发方向。"),
    ("acceptance", "安装与功能检查", "软件、采样、外设与手机接入的检查表。"),
    ("troubleshooting", "排障与恢复", "从电源、串口、构建、显示、HTTP 到 BLE 的故障诊断。"),
]


def public_markdown(text: str, source: Path, source_ref: str) -> str:
    """Keep handbook links local and make other repository references explicit."""
    public_paths = {p.relative_to(ROOT) for p in source_files()}
    def replace(match: re.Match[str]) -> str:
        label, target = match.group(1), match.group(2)
        if re.match(r"^(https?://|mailto:|#|/)", target):
            return match.group(0)
        path_text, _, fragment = target.partition("#")
        resolved = (source.parent / path_text).resolve()
        try:
            relative = resolved.relative_to(ROOT)
        except ValueError:
            raise ValueError(f"Out-of-project link in {source.name}: {target}")
        if relative.parts[:2] == ("docs", "handbook"):
            destination = PUBLIC_BASE + ("/" if resolved.stem == "index" else "/" + resolved.stem)
            if fragment:
                destination += "#" + fragment
        elif relative.as_posix() == "docs/wiring.md":
            destination = PUBLIC_BASE + "/hardware-wiring"
            if fragment:
                destination += "#" + fragment
        elif relative.parts[:2] == ("web", "experience"):
            destination = "/roboplanter/" + Path(*relative.parts[2:]).as_posix()
        elif relative in public_paths:
            destination = "/roboplanter/source/" + relative.as_posix() + ".txt"
        elif relative.parts[:2] == ("docs", "deployment") or relative.parts[0] == "reference":
            if source.stem == "roadmap-evidence":
                return label + "（原始记录未公开；本页为摘要）"
            destination = PUBLIC_BASE + "/roadmap-evidence"
        else:
            kind = "tree" if resolved.is_dir() else "blob"
            destination = f"https://github.com/peterpanstechland/RoboPlanter/{kind}/{source_ref}/{relative.as_posix()}"
            if fragment:
                destination += "#" + fragment
        return f"[{label}]({destination})"
    return re.sub(r"\[([^\]]+)\]\(([^\s)]+)\)", replace, text)


def build_device_kit_bundle(destination: Path) -> dict:
    files = [p for p in source_files() if p.relative_to(ROOT).as_posix().startswith("integrations/device-kit/") or p.relative_to(ROOT).as_posix() in {"tests/test_device_kit.py", "docs/handbook/device-platform.md"}]
    index = {}
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
        for path in files:
            name = path.relative_to(ROOT).as_posix()
            data = path.read_bytes()
            write_member(bundle, name, data)
            index[name] = hashlib.sha256(data).hexdigest()
        write_member(bundle, "README-START-HERE.txt", b"RoboPlanter Device Kit\nPython 3.10+, standard library only.\nRun: python3 integrations/device-kit/device_kit.py validate integrations/device-kit/profiles/*.json\nRun tests: python3 -m unittest discover -s tests -p test_device_kit.py -v\nThis is an offline toolkit. No device connection, credential loading, or commands are supported.\nDocs: https://peterpanstechland.github.io/docs/projects/2026/roboplanter/device-platform\n")
        write_member(bundle, "SOURCE-MANIFEST.json", (json.dumps(index, indent=2, ensure_ascii=False) + "\n").encode())
    return {"filename": destination.name, "sha256": hashlib.sha256(destination.read_bytes()).hexdigest(), "files": len(files)}


def sync(site: Path, source_ref: str) -> dict:
    site = site.resolve()
    if not (site / "docusaurus.config.ts").is_file() or not (site / "package.json").is_file():
        raise ValueError("--site must be an existing Docusaurus checkout")
    if site == ROOT:
        raise ValueError("The docs site must be a separate checkout")
    out = site / "docs/projects/2026/roboplanter"
    out.mkdir(parents=True, exist_ok=True)
    zh = site / "i18n/zh-Hans/docusaurus-plugin-content-docs/current/projects/2026/roboplanter"
    zh.mkdir(parents=True, exist_ok=True)
    # English is the site's default locale. Never silently copy Chinese into it.
    for position, (slug, title, description) in enumerate(PAGES, 1):
        english = ROOT / "docs/handbook/en" / f"{slug}.md"
        if not english.is_file():
            raise ValueError(f"Missing English handbook page: {slug}")
        chinese = ROOT / "docs/wiring.md" if slug == "hardware-wiring" else ROOT / "docs/handbook" / f"{slug}.md"
        en_text = english.read_text(encoding="utf-8")
        en_title = en_text.splitlines()[0].removeprefix("# ")
        if re.search(r"[\u4e00-\u9fff]", en_title):
            raise ValueError(f"English heading contains untranslated Chinese: {slug}")
        en_description = next((line for line in en_text.splitlines()[1:] if line and line[0] not in "#>|![`"), en_title)[:200]
        for locale, source, destination, label, summary in [
            ("en", english, out, en_title, en_description),
            ("zh-Hans", chinese, zh, title, description),
        ]:
            content = public_markdown(source.read_text(encoding="utf-8"), source, source_ref)
            if locale == "zh-Hans":
                content = content.replace(PUBLIC_BASE, "/zh-Hans" + PUBLIC_BASE)
            front = f"---\ntitle: {json.dumps(label, ensure_ascii=False)}\ndescription: {json.dumps(summary, ensure_ascii=False)}\nsidebar_position: {position}\nhide_title: true\n"
            if slug == "index":
                front += "slug: /projects/2026/roboplanter\n"
            (destination / f"{slug}.md").write_text(front + "---\n\n" + content, encoding="utf-8")
    for destination, label in [(out, "RoboPlanter · Desk Companion"), (zh, "工位搭子 · RoboPlanter")]:
        (destination / "_category_.json").write_text(json.dumps({"label": label, "position": 1, "link": {"type": "doc", "id": "index"}}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    # Docusaurus translates generated sidebar categories through current.json.
    # A localized _category_.json alone does not translate the English label.
    translations_path = site / "i18n/zh-Hans/docusaurus-plugin-content-docs/current.json"
    translations = json.loads(translations_path.read_text(encoding="utf-8"))
    translations["sidebar.docsSidebar.category.RoboPlanter · Desk Companion"] = {
        "message": "YOYO工位绿植搭子",
        "description": "The RoboPlanter project category in sidebar docsSidebar",
    }
    translations_path.write_text(json.dumps(translations, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    static = site / "static/roboplanter"
    if not static.resolve().is_relative_to(site) or static.resolve() != site / "static/roboplanter":
        raise ValueError("Static target must resolve inside the selected checkout")
    marker = static / ".roboplanter-managed"
    if static.exists():
        if static.is_symlink() or not marker.is_file():
            raise ValueError("Refusing to replace an unowned static/roboplanter directory")
        shutil.rmtree(static)
    shutil.copytree(ROOT / "web/experience", static, dirs_exist_ok=True, ignore=shutil.ignore_patterns("README.md"))
    marker.write_text("managed by tools/sync_docs_site.py\n", encoding="utf-8")
    downloads = static / "downloads"
    downloads.mkdir(exist_ok=True)
    kit = build_device_kit_bundle(downloads / "roboplanter-device-kit.zip")
    starter = package_starter(downloads / "roboplanter-starter.zip")
    honor_skill = package_honor_skill(downloads / "roboplanter-honor-skill.zip")
    print_files = [ROOT / "design/print" / f"Body{n}.3mf" for n in (8, 9, 16)]
    if not all(path.is_file() for path in print_files):
        raise ValueError("Missing printable Body8, Body9 or Body16")
    print_zip = downloads / "roboplanter-3d-print.zip"
    with zipfile.ZipFile(print_zip, "w", compression=zipfile.ZIP_DEFLATED) as bundle:
        for path in print_files:
            write_member(bundle, path.name, path.read_bytes())
        write_member(bundle, "PRINT-LIST.txt", b"Print Body8, Body9 and Body16, one each. Body20 is a reference volume, not a printed part. Original manufacturing geometry is unchanged. Inspect slicing, fits, support and liquid/electronics separation for your own printer.\n")
    prints = {"filename": print_zip.name, "files": 3, "sha256": hashlib.sha256(print_zip.read_bytes()).hexdigest()}
    for path in source_files():
        copy = static / "source" / (path.relative_to(ROOT).as_posix() + ".txt")
        copy.parent.mkdir(parents=True, exist_ok=True)
        copy.write_bytes(path.read_bytes())
    (downloads / "manifest.json").write_text(json.dumps({"device_kit": kit, "starter": starter, "honor_skill": honor_skill, "print_models": prints}, indent=2) + "\n", encoding="utf-8")
    block = '\n<!-- ROBOPLANTER:START -->\n### [YOYO工位绿植搭子 · RoboPlanter](/docs/projects/2026/roboplanter/)\n\n你先下班，我负责发芽。一盆真实绿植、一只像素精灵、一张可以带走的工位日签。荣耀 YOYO 负责对话，XIAO 协调传感、灯光与蓝牙打印，Indicator 让陪伴常驻桌面。\n\n[先看懂作品](/docs/projects/2026/roboplanter/) · [互动体验](https://peterpanstechland.github.io/roboplanter/) · [DIY 制作](/docs/projects/2026/roboplanter/hardware-quickstart) · [工程 API](/docs/projects/2026/roboplanter/system-api) · [公开源码](https://github.com/peterpanstechland/RoboPlanter)\n\n**实机依据截至 2026-11-04：**手机校准百分比、保留绑定重连、30 灯珠绿→白→绿、D7 测距，以及屏幕预览到蓝牙打印和自动切纸已确认。完整手机场景、语音计时与实际换网仍在联调；[查看功能状态](/docs/projects/2026/roboplanter/roadmap-evidence)。\n<!-- ROBOPLANTER:END -->\n'
    en_block = "\n<!-- ROBOPLANTER:START -->\n### [RoboPlanter · A Talking Desk Garden](/docs/projects/2026/roboplanter/)\n\nYou clock out. I'll keep growing. A real plant, a pixel companion and a small paper ritual, built with HONOR YOYO, XIAO and SenseCAP Indicator.\n\n[English experience](https://peterpanstechland.github.io/roboplanter/en/) · [Build tutorial](/docs/projects/2026/roboplanter/hardware-quickstart) · [System API](/docs/projects/2026/roboplanter/system-api) · [Source](https://github.com/peterpanstechland/RoboPlanter)\n\n**Field evidence (November 4, 2026):** calibrated YOYO soil percentages, bond-preserving reconnect, 30-pixel status/fill light, D7 ultrasonic readings and screen-preview-to-BLE-printer output with correct proportions and automatic cutting confirmed. Full phone scene-print/focus and network-change acceptance remain outstanding. The web preview never controls hardware.\n<!-- ROBOPLANTER:END -->\n"
    indexes = [(site / "docs/projects/index.md", en_block), (site / "i18n/zh-Hans/docusaurus-plugin-content-docs/current/projects/index.md", block.replace(PUBLIC_BASE, "/zh-Hans" + PUBLIC_BASE))]
    for path, localized_block in indexes:
        original = path.read_text(encoding="utf-8")
        if "<!-- ROBOPLANTER:START -->" in original:
            updated = re.sub(r"\n?<!-- ROBOPLANTER:START -->.*?<!-- ROBOPLANTER:END -->\n?", localized_block, original, flags=re.S)
        else:
            # Keep all existing portfolio content and insert just after front matter.
            boundary = original.find("\n---", 4) if original.startswith("---") else -1
            boundary = original.find("\n", boundary + 1) + 1 if boundary >= 0 else 0
            updated = original[:boundary] + localized_block + original[boundary:]
        path.write_text(updated, encoding="utf-8")
    return {"site": str(site), "pages_per_locale": len(PAGES), "locales": ["en", "zh-Hans"], "device_kit": kit, "starter": starter, "honor_skill": honor_skill}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site", type=Path, required=True)
    parser.add_argument("--source-ref", default="codex/initial-sync")
    args = parser.parse_args()
    print(json.dumps(sync(args.site, args.source_ref), ensure_ascii=False, indent=2))
