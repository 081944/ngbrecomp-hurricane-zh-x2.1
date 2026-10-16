"""--ebp-to-callee: a callee that reads the caller's ebp.

A lifted callee that inherits a frame reads ebp from g_ebp / g_seh_ebp, which a function sets in its
prologue. In an FPO caller ebp is a plain register (here the menu state the patched-in code writes
through [ebp+0xE]), so the callee read an older frame. With the call site in lifter.ebp_to_callee the
caller's ebp is handed over around the call and the inherited values put back after; a call site
naming another callee is refused.

Run: python -m tools.recomp.test_ebp_to_callee
"""
import unittest

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

from .disasm import BasicBlock, Disassembler
from .lifter import Lifter, lift_basic_block

_CS = Cs(CS_ARCH_X86, CS_MODE_32)
_CS.detail = True
_D = Disassembler()

# 0x1000: mov ebp, 0x5000   0x1005: call 0x2000   0x100A: mov ax, [ebp+0xE]
_CODE = "bd00500000" + "e8f60f0000" + "668b450e"


def _lift(ebp_to_callee=None):
    lifter = Lifter()
    if ebp_to_callee is not None:
        lifter.ebp_to_callee = ebp_to_callee
    insns = [_D._decode_instruction(i) for i in _CS.disasm(bytes.fromhex(_CODE), 0x1000)]
    assert [i.mnemonic for i in insns] == ["mov", "call", "mov"], [i.mnemonic for i in insns]
    out, _ = lift_basic_block(lifter, BasicBlock(start=0x1000, instructions=insns))
    return "\n".join(out)


class EbpToCalleeTest(unittest.TestCase):
    def test_the_callee_gets_the_callers_ebp_and_the_frame_comes_back(self):
        text = _lift({0x1005: 0x2000})
        before, call, after = text.partition("RECOMP_ABI_CALL(0x00002000u")
        self.assertTrue(call)
        self.assertIn("g_ebp = g_seh_ebp = ebp;", before.splitlines()[-2] + before.splitlines()[-1])
        self.assertIn("g_ebp = _ebp_in; g_seh_ebp = _seh_in; }", after)
        self.assertLess(after.index("_seh_in; }"), after.index("ebp + 0xE"))   # put back before the caller goes on

    def test_without_the_map_the_call_is_as_before(self):
        text = _lift()
        self.assertNotIn("_seh_in", text)
        self.assertEqual(text, _lift({0x9000: 0x2000}))

    def test_a_site_naming_another_callee_is_refused(self):
        with self.assertRaises(SystemExit):
            _lift({0x1005: 0x3000})


if __name__ == "__main__":
    unittest.main()
