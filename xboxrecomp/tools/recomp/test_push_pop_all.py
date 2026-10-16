"""
Self-check for pushad / popad / pushfd / popfd.

Run: py -3 tools/recomp/test_push_pop_all.py   (CC=<a C compiler> also runs the lifted statements)

They used to lift to RECOMP_UNIMPL, which the runtime logs and skips: a patched-in hook of the shape
`pushfd; pushad; ... mov [esp+0x1C], eax ...; popad; popfd; ret` then wrote its [esp+n] slots into the
caller's frame and restored no register. Checked here: the text of each, and -- with a compiler -- that
running them on a simulated stack gives the x86 layout (eax ecx edx ebx esp-before ebp esi edi, from
high to low addresses), that popad skips the saved esp and restores the rest, and that pushfd / popfd
leave esp where it was.
"""

import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

from tools.recomp.lifter import Lifter, _EFLAGS_PRESERVE, _FLAGS_UNDEFINED  # noqa: E402


class _Insn:
    def __init__(self, mnemonic, address=0x00DC8000, size=1):
        self.mnemonic = mnemonic
        self.operands = []
        self.op_str = ""
        self.address = address
        self.size = size
        self.call_target = None

    @property
    def end_address(self):
        return self.address + self.size


def lift(m):
    return "\n".join(Lifter().lift_instruction(_Insn(m)))


def test_text():
    for m in ("pushal", "popal", "pushfd", "popfd"):
        out = lift(m)
        assert "RECOMP_UNIMPL" not in out, (m, out)
    assert "PUSH32(esp, _pa_sp)" in lift("pushal")
    assert "esp += 4;" in lift("popal") and "POP32(esp, eax)" in lift("popal")
    assert lift("pushfd").startswith("PUSH32(esp,")
    assert lift("popfd").startswith("esp += 4;")


def test_flag_tracking():
    # pushad / popad / pushfd leave EFLAGS alone; popfd replaces them with unknown values
    assert {"pushfd", "pushal", "popal"} <= _EFLAGS_PRESERVE
    assert "popfd" in _FLAGS_UNDEFINED and "popfd" not in _EFLAGS_PRESERVE


HARNESS = r"""
#include <stdint.h>
#include <stdio.h>
static uint32_t ram[1024];
#define MEM32(a) ram[((a) - 0x1000u) / 4]
#define PUSH32(sp, val) do { uint32_t _pv = (uint32_t)(val); (sp) -= 4; MEM32(sp) = _pv; } while (0)
#define POP32(sp, dst) do { (dst) = MEM32(sp); (sp) += 4; } while (0)
uint32_t eax = 0xA, ecx = 0xC, edx = 0xD, ebx = 0xB, esp = 0x1800, ebp = 0xBB, esi = 0x51, edi = 0xD1;
int main(void)
{
    int bad = 0;
    uint32_t sp0 = esp;
    @PUSHFD@
    @PUSHAL@
    /* x86 layout: [esp] = edi ... [esp+0x1C] = eax, [esp+0x0C] = esp before pushad */
    uint32_t want[8] = { 0xD1, 0x51, 0xBB, sp0 - 4, 0xB, 0xD, 0xC, 0xA };
    for (int i = 0; i < 8; i++)
        if (MEM32(esp + 4 * i) != want[i]) { printf("slot %d: %08x want %08x\n", i, MEM32(esp + 4 * i), want[i]); bad = 1; }
    if (esp != sp0 - 36) { printf("esp after pushes %08x\n", esp); bad = 1; }
    MEM32(esp + 0x1C) = 0x12345678;            /* a hook returning a value through the saved eax */
    eax = ecx = edx = ebx = ebp = esi = edi = 0xDEAD;
    MEM32(esp + 0x0C) = 0xBADBAD;              /* popad must ignore the saved esp */
    @POPAL@
    @POPFD@
    if (eax != 0x12345678 || ecx != 0xC || edx != 0xD || ebx != 0xB || ebp != 0xBB || esi != 0x51 || edi != 0xD1) {
        printf("regs %x %x %x %x %x %x %x\n", eax, ecx, edx, ebx, ebp, esi, edi); bad = 1; }
    if (esp != sp0) { printf("esp at end %08x want %08x\n", esp, sp0); bad = 1; }
    puts(bad ? "FAIL" : "ok");
    return bad;
}
"""


def test_semantics():
    cc = os.environ.get("CC")
    if not cc:
        print("test_semantics: skipped (set CC to run the lifted statements)")
        return
    src = (HARNESS.replace("@PUSHFD@", lift("pushfd")).replace("@PUSHAL@", lift("pushal"))
           .replace("@POPAL@", lift("popal")).replace("@POPFD@", lift("popfd")))
    with tempfile.TemporaryDirectory() as d:
        c, exe = os.path.join(d, "t.c"), os.path.join(d, "t.exe")
        open(c, "w").write(src)
        subprocess.run([cc, "-O2", "-Wall", "-o", exe, c], check=True)
        r = subprocess.run([exe], capture_output=True, text=True)
        assert r.returncode == 0, r.stdout + r.stderr
        assert r.stdout.strip() == "ok", r.stdout


if __name__ == "__main__":
    test_text()
    test_flag_tracking()
    test_semantics()
    print("ok")
