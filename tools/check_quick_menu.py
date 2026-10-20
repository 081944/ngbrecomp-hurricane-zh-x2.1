#!/usr/bin/env python3
"""Read-only source preflight after porting the quick menu to another revision.

This is an integration check, not a substitute for compiling and playtesting.
"""
import json
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
errors = []

def require(path, *tokens):
    p = root / path
    if not p.is_file():
        errors.append(f"missing {path}")
        return ""
    text = p.read_text(encoding="utf-8")
    for token in tokens:
        if token not in text:
            errors.append(f"{path}: missing integration {token!r}")
    return text

manifest = json.loads(require("quick-menu-compatibility.json"))
manual = json.loads(require("src/manual_functions.json"))
if manual.count(manifest["guest_hook"]) != 1:
    errors.append("manual hook must be registered exactly once")
require("src/quick_select.c", "void sub_00166960(void)", "native_pause_check();",
        "supported_layout()", "call(sub_002FD820)", "call(sub_00166B90)")
require("CMakeLists.txt", "src/quick_select.c", "src/quick_ui.c")
require("src/main.c", "ngb_quick_poll()")
require("src/cheats.c", "ngb_quick_tick();")
require("src/d3d/vk/backend.cpp", '#include "quick_menu.inc"', "QuickMenu(*show)")
require("src/d3d/vk/quick_menu.inc", "ngb_quick_layer(pixels)", "VK_ATTACHMENT_LOAD_OP_LOAD",
        "if (ngb_quick_frozen()) QuickMenuBackground(dst);", "d_.vkCmdBlitImage(")
require("src/quick_ui.c", "s_navigation++", "int ngb_quick_take_navigation(void)")
require("src/quick_ui.c", "else if (!s_block && (edge & (Q_UP | Q_DOWN)))",
        "s_request = edge & Q_DOWN ? NGB_QUICK_ITEMS : NGB_QUICK_MELEE;",
        "* 4489 / 10000;")
require("src/quick_select.c", "ngb_quick_take_navigation()", "0xA1", "0xA6")
require("src/launcher.c", manifest["persistent_pause_key"])
require("src/launcher.c", manifest["persistent_scale_key"], "s->quick_menu_scale = 100;",
        "ngb_quick_scale_value", "ngb_quick_scale_step")
require("src/ingame_menu.c", "case 19: return s_running.quick_menu_scale;",
        "case 19: s_running.quick_menu_scale = ngb_quick_scale_value(value); break;")
require("src/pc_menu.c", "ROW_FPS, ROW_QUICKSCALE, ROW_BACK", "Quick Menu Scale",
        'T_QUICKSCALE0 + 10, "Off"', "case ROW_QUICKSCALE: src = 0x429984; max = 10;")
require("src/pc_menu.c", *[f'"{v}%"' for v in manifest["scale_percentages"] if v])
require("src/pause_tab.c", "S_FPS, S_QUICKSCALE, -1", "Quick Menu Scale")
require("src/d3d/vk/quick_menu.inc", "int percent = ngb_pc_setting(19);", "if (!percent) return;",
        "float scale = percent / 100.0f;",
        "sx *= scale; sy *= scale;")
require("src/quick_select.c", "if (!ngb_pc_setting(19))", "ngb_quick_take_open();")
require("src/quick_ui.c", "if (!ngb_pc_setting(19)) return 0;",
        "s_open = s_block = s_command = s_request = s_edges = s_navigation = 0;")
if "Waiting for action" in require("src/quick_ui.c"):
    errors.append("quick-menu waiting text must remain hidden")
require("src/ingame_menu.c", "ngb_quick_event(")
require("src/pause_tab.c", "Quick Menu Pause")
require("src/pc_menu.c", "Quick Menu Pause")
require("src/bindings.c", *manifest["persistent_actions"])
require("src/bindings.h", "A_QUICK_WEAPONS", "A_QUICK_ITEMS", "A_QUICK_LEFT", "A_QUICK_RIGHT")
require("src/bindings.h", "A_QUICK_CONFIRM", "A_QUICK_CANCEL")
require("src/quick_select.h", "NGB_QUICK_CATEGORIES = 4", "int category, ready;")
require("src/quick_select.c", "int ready = can_pause();", "make_menu(&menu, requested, ready)",
        "ngb_quick_take_command(&item, &category) && ready", "cls < 1 || cls > 3")
require("src/quick_ui.c", "s_open && s_menu.ready &&", "s_menu.ready && s_confirm_ready", "Q_UP | Q_DOWN | Q_CONFIRM")
# The HLSL register mapper in vk/spirv.cpp maps one resource declaration per
# line. An unmapped sampler can alias a uniform buffer and lose the device.
layer = require("src/d3d/vk/quick_menu.inc")
for literal in re.findall(r'"((?:\\.|[^"\\])*)"', layer):
    for line in literal.split(r"\n"):
        if line.count("register(") > 1:
            errors.append("GPU layer has multiple resource declarations on one HLSL line")
for error in errors:
    print(error, file=sys.stderr)
if errors:
    sys.exit(1)
print("Quick menu source integration passed. Compile and run the documented runtime checks.")
print("Recorded upstream:", manifest["tested_upstream_commit"])
