"""
Self-check for code that only a direct branch reaches, past bytes the linear
decode stops at.

Run: py -3 tools/recomp/test_branch_only_tail.py

disassemble_function decodes a function linearly and capstone stops at the first
byte sequence that is not an instruction. A switch's jump table and index bytes in
the middle of a function are usually one, and code placed after them -- a tail
several cases share, after a run of int3 -- then never gets decoded. Its label is
missing, and the translator drops the branches to it as dead code, so the cases
fall through into one another.

In Ninja Gaiden Black the particle-script property getter at 0x000FAA10 keeps
such a tail at 0x000FAB50: the cases for a particle's owner, target and parent
jump there to return the object's variant field. Without it the Gobdeck's fire
breath got no flames. The decode now restarts at every branch target it has not
decoded.
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

from tools.recomp import config  # noqa: E402
from tools.recomp.translator import FunctionTranslator  # noqa: E402

try:
    import pytest
except ImportError:  # this file also runs as a plain script
    pytest = None

if pytest is not None:
    @pytest.fixture(autouse=True)
    def _restore_config_after_test():
        """config._install changes process globals; do not leak them into test_config."""
        yield
        import importlib
        importlib.reload(config)

BASE = 0x00010000

# jmp +6 (to BASE+8)                     -> EB 06
# 0F 00 F0: "group 6 /6", no such instruction: the linear decode stops here
# int3 x3: padding before the tail
# xor eax, eax; ret                      -> 31 C0 C3
IMAGE = bytes.fromhex("EB06" "0F00F0" "CCCCCC" "31C0" "C3")
TAIL = BASE + 8


def _setup(image):
    config._install(
        [config.Section(".text", BASE, len(image), 0x0000, len(image), True)],
        entry_point=BASE, kernel_thunk_addr=BASE, origin="branch-only-tail-test")
    return image


def _func_db(placements):
    db = {}
    for va, size in placements:
        db[va] = {"start": f"0x{va:08X}", "end": va + size,
                  "_addr": va, "size": size}
    return db


def test_tail_past_undecodable_bytes_is_lifted():
    image = _setup(IMAGE)
    db = _func_db([(BASE, len(IMAGE))])
    c = FunctionTranslator(image, db).translate_function(BASE, db[BASE])
    assert f"loc_{TAIL:08X}:" in c, c
    assert "dead code, label not in function" not in c, c
    assert f"goto loc_{TAIL:08X};" in c, c
    print("ok  tail_past_undecodable_bytes_is_lifted")


# A switch whose first arm jumps to such a tail. The arms are reached through
# the table alone, so the decode only follows the arm's jump when the arm is
# one of its roots -- which the translator must make it even when the linear
# pass already decoded it (sub_000FAA10's cases were all before its tables).
#
#   and eax, 3; jmp [eax*4 + TABLE-4]   (index-from-1 table, as MSVC emits)
#   table: three arms
#   arm1: jmp TAIL           arm2, arm3: mov eax, n; ret
#   0F 00 F0; int3; TAIL: xor eax, eax; ret
SW_TABLE = BASE + 12
SW_ARMS = [BASE + 24, BASE + 26, BASE + 32]
SW_TAIL = BASE + 42
SW_IMAGE = (bytes.fromhex("83E003") + b"\xFF\x24\x85" + (SW_TABLE - 4).to_bytes(4, "little")
            + b"\x90\x90"
            + b"".join(a.to_bytes(4, "little") for a in SW_ARMS)
            + bytes.fromhex("EB10")
            + bytes.fromhex("B802000000C3") + bytes.fromhex("B803000000C3")
            + bytes.fromhex("0F00F0") + b"\xCC"
            + bytes.fromhex("31C0C3"))
assert len(SW_IMAGE) == 45 and SW_IMAGE[24:26] == b"\xEB\x10"


def test_switch_arm_jump_to_tail_is_lifted():
    image = _setup(SW_IMAGE)
    db = _func_db([(BASE, len(SW_IMAGE))])
    c = FunctionTranslator(image, db).translate_function(BASE, db[BASE])
    assert "switch: 3 entries" in c, c
    assert f"loc_{SW_TAIL:08X}:" in c, c
    assert "dead code, label not in function" not in c, c
    print("ok  switch_arm_jump_to_tail_is_lifted")


if __name__ == "__main__":
    test_tail_past_undecodable_bytes_is_lifted()
    test_switch_arm_jump_to_tail_is_lifted()
