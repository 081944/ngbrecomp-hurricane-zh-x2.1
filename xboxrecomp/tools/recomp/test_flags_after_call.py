"""--flags-after-call: a callee that hands its flags back to the caller.

Lifted functions keep flags to themselves (_fa/_fb are locals), so after

    cmp  al, 1
    call hook          ; patched-in code that ends `cmp al, 0xFF; ret`
    jne  target

the jne was resolved from `cmp al, 1`, the caller's own last compare. With the call site in
lifter.flags_after_call the callee's compare is lifted again after the call and becomes the
tracked setter, so the jne reads 0xFF. A call not in the map, and the map left empty, still
resolve from the caller's compare (the positive control: a lifter that resolved nothing
would pass the first test).

Run: python -m tools.recomp.test_flags_after_call
"""
import unittest

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

from .disasm import BasicBlock, Disassembler
from .lifter import Lifter, lift_basic_block

_CS = Cs(CS_ARCH_X86, CS_MODE_32)
_CS.detail = True
_D = Disassembler()


def _decode(hexbytes, va):
    return [_D._decode_instruction(i) for i in _CS.disasm(bytes.fromhex(hexbytes), va)]


# 0x1000: cmp al,1   0x1002: call 0x2000   0x1007: jne 0x1040
_CODE = "3c01" + "e8f90f0000" + "7537"


def _lift(flags_after_call=None):
    lifter = Lifter()
    if flags_after_call is not None:
        lifter.flags_after_call = flags_after_call
    insns = _decode(_CODE, 0x1000)
    assert [i.mnemonic for i in insns] == ["cmp", "call", "jne"], [i.mnemonic for i in insns]
    out, state = lift_basic_block(lifter, BasicBlock(start=0x1000, instructions=insns))
    return "\n".join(out), state


class FlagsAfterCallTest(unittest.TestCase):
    def test_branch_reads_the_callees_compare(self):
        after = _decode("3cff", 0x1007)[0]
        text, state = _lift({0x1002: after})
        tail = text.split("call 0x00002000")[1]
        self.assertIn("0xFF", tail)              # the redone compare, after the call
        self.assertIn("CMP_NE", tail)            # and the jne resolved from it
        self.assertNotIn("(1) & 0xFF", tail)     # not from the caller's cmp al, 1

    def test_without_the_map_the_callers_compare_still_decides(self):
        text, _ = _lift()
        tail = text.split("call 0x00002000")[1]
        self.assertIn("CMP_NE", tail)
        self.assertNotIn("0xFF) & 0xFF", tail)

    def test_other_call_sites_are_untouched(self):
        after = _decode("3cff", 0x5005)[0]
        self.assertEqual(_lift({0x5000: after})[0], _lift()[0])


if __name__ == "__main__":
    unittest.main()
