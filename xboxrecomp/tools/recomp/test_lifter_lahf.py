"""lahf after ucomiss: MSVC's `ucomiss x, 0; lahf; test ah, 0x44; jnp` is its
"is this float zero" test. lahf used to lift to a comment, so AH kept whatever
EAX held and the guard answered at random -- Burnout 3 divided 0 by 0 behind
one and put a NaN in the player's position."""
import unittest

from .disasm import BasicBlock, Instruction, Operand
from .lifter import Lifter, lift_basic_block


def _xmm(n):
    return Operand(type="reg", reg="xmm%d" % n)


def _block():
    cmp = Instruction(0, 3, "ucomiss", "xmm0, xmm4", "0f2ec4")
    cmp.operands = [_xmm(0), _xmm(4)]
    lahf = Instruction(3, 1, "lahf", "", "9f")
    lahf.operands = []
    return BasicBlock(start=0, instructions=[cmp, lahf])


class LahfTest(unittest.TestCase):
    def test_lahf_loads_ah_from_the_compare(self):
        out = "\n".join(lift_basic_block(Lifter(), _block())[0])
        self.assertIn("eax = (eax & 0xFFFF00FFu)", out)
        self.assertNotIn("/* lahf - load AH", out)
        # ZF (0x40) must be set on equal AND on unordered, as the hardware does.
        self.assertIn("(_fca == _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x40u", out)
        # PF (0x04) is the unordered bit; CF (0x01) is below-or-unordered.
        self.assertIn("((_fca != _fca || _fcb != _fcb)) ? 0x04u", out)
        self.assertIn("0x02u", out)


class FpuUnorderedTest(unittest.TestCase):
    """fcomi/fucomi/sahf set ZF, PF and CF on an unordered compare; g_fp_cmp
    is 2 then, which must not read as "greater"."""

    def test_ja_is_false_and_jb_true_when_unordered(self):
        from .lifter import _make_condition
        ja = _make_condition("ja", "fucomip", [])[0]
        jb = _make_condition("jb", "fucomip", [])[0]
        jp = _make_condition("jp", "sahf", [])[0]
        self.assertIn("g_fp_cmp == 1", ja)
        self.assertNotIn(">", ja)
        self.assertIn("g_fp_cmp == 2", jb)
        self.assertIn("g_fp_cmp == 2", jp)


class LahfAfterArithmeticTest(unittest.TestCase):
    """lahf after an add reads CF from _cf, so the function must declare it.
    It was not counted as a CF consumer and the C failed to compile."""

    def test_lahf_after_add_declares_cf(self):
        from .translator import FunctionTranslator
        add = Instruction(0, 2, "add", "eax, ecx", "01c8")
        add.operands = [Operand(type="reg", reg="eax"),
                        Operand(type="reg", reg="ecx")]
        lahf = Instruction(2, 1, "lahf", "", "9f")
        lahf.operands = []
        self.assertTrue(FunctionTranslator._function_needs_cf([add, lahf]))


if __name__ == "__main__":
    unittest.main()
