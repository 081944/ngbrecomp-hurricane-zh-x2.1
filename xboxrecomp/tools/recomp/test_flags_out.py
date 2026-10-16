"""--flags-out: a function that returns its flags.

    0x2000: test ecx, ecx      ; the answer
            popad              ; restores the caller's ecx: redoing the test in the caller reads that
            ret
    0x1000: cmp  ebx, 2
            call 0x2000
            je   target        ; branches on 0x2000's test, not on cmp ebx, 2

With 0x2000 in lifter.flags_out the function stores the compare's snapshot (g_flags_out_*) before its
ret and the caller loads it back after the call, tracking the test as the setter, so the je reads it.
Without the map both lift as before (the positive control: the je still resolves, from cmp ebx, 2).

Run: python -m tools.recomp.test_flags_out
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


_CALLEE = "85c9" + "61" + "c3"                        # 0x2000: test ecx,ecx; popad; ret
_CALLER = "83fb02" + "e8f80f0000" + "7437"           # 0x1000: cmp ebx,2; call 0x2000; je 0x1040


def _lift(code, va, flags_out=None, func_start=None):
    lifter = Lifter()
    if flags_out is not None:
        lifter.flags_out = flags_out
    lifter.func_start = func_start if func_start is not None else va
    insns = _decode(code, va)
    out, _ = lift_basic_block(lifter, BasicBlock(start=va, instructions=insns))
    return "\n".join(out)


def _map():
    return {0x2000: _decode("85c9", 0x2005)[0]}


class FlagsOutTest(unittest.TestCase):
    def test_callee_publishes_before_ret(self):
        text = _lift(_CALLEE, 0x2000, _map())
        publish, ret = text.index("g_flags_out_fa = _fa;"), text.index("return; /* ret */")
        self.assertLess(text.index("/* cmp ecx, 0"), publish)      # after the compare (test ecx,ecx)
        self.assertLess(text.index("/* popad */"), publish)        # and after popad: the snapshot is
        self.assertLess(publish, ret)                               # the compare's, popad leaves it

    def test_caller_takes_the_flags_back(self):
        text = _lift(_CALLER, 0x1000, _map())
        tail = text.split("call 0x00002000")[1]
        self.assertIn("_fa = g_flags_out_fa;", tail)
        self.assertLess(tail.index("_fa = g_flags_out_fa;"), tail.index("CMP_EQ"))

    def test_without_the_map_nothing_changes(self):
        callee = _lift(_CALLEE, 0x2000)
        caller = _lift(_CALLER, 0x1000)
        self.assertNotIn("g_flags_out", callee + caller)
        self.assertIn("CMP_EQ", caller.split("call 0x00002000")[1])   # still from cmp ebx, 2

    def test_other_functions_do_not_publish(self):
        self.assertNotIn("g_flags_out", _lift(_CALLEE, 0x3000, _map()))


if __name__ == "__main__":
    unittest.main()
