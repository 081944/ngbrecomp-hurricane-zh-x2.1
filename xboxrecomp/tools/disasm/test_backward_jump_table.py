"""A switch table indexed backwards is still a table.

MSVC's CRT memmove copies its trailing dwords with

    neg ecx
    jmp dword ptr [ecx*4 + TrailDownVec_end]

so the displacement names the table's *last* slot and the arms live below it.
resync_jump_tables counted entries forward from the displacement, found one,
called it too short to be a table and left the linear sweep decoding the
pointers as instructions. The function then ended at the table and every arm
after it came out as an "indirect call" to code that was not a function
(Ninja Gaiden Black, sub_0036EB10).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

from tools.disasm.test_returning_body_switch import BASE, _engine  # noqa: E402


def _negated_dispatch():
    # +0:  neg ecx                          f7 d9
    # +2:  jmp dword ptr [ecx*4 + END]      ff 24 8d <END>
    # +9:  lea ecx, [ecx]  (npad 3)         8d 49 00
    # +12: TABLE: arm3, arm2, arm1          (indices -3..-1)
    # +24: END:   arm0                      (index 0)
    # +28: arm3..arm0: one dword copy each, falling through, then ret
    arms = BASE + 28
    end = BASE + 24
    data = b"\xf7\xd9" + b"\xff\x24\x8d" + struct.pack("<I", end) + b"\x8d\x49\x00"
    # mov eax,[esi+ecx*4+0xc] / mov [edi+ecx*4+0xc],eax is 8 bytes per arm
    data += struct.pack("<IIII", arms, arms + 8, arms + 16, arms + 24)
    for disp in (0x0C, 0x08, 0x04):
        data += b"\x8b\x44\x8e" + bytes([disp]) + b"\x89\x44\x8f" + bytes([disp])
    data += b"\x8b\x06\x89\x07\x90\x90\x90\x90"   # arm0: mov eax,[esi]; mov [edi],eax
    data += b"\xc3"
    return data, end, arms


def test_negated_index_table_is_measured_from_its_start():
    data, end, arms = _negated_dispatch()
    engine, _ = _engine(data + b"\xcc" * 16)
    start = BASE + 12
    assert engine.jump_tables.get(start) == end + 4
    assert engine.jump_table_entries(end) == [arms, arms + 8, arms + 16, arms + 24]
    # The arms decode as the copies they are, in phase.
    assert arms in engine.instructions
    assert engine.instructions[arms].mnemonic == "mov"


def test_a_forward_table_does_not_reach_backwards():
    # Same layout without the neg: the displacement is an ordinary table base
    # and the dword before it is not part of it, pointer-shaped or not.
    data, end, arms = _negated_dispatch()
    data = b"\x90\x90" + data[2:]
    engine, _ = _engine(data + b"\xcc" * 16)
    assert BASE + 12 not in engine.jump_tables
