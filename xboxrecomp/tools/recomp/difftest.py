"""Differential test of one lifted function against the original machine code.

    python -m tools.recomp.difftest XBE CAPTURE_DIR VA [--tol 1e-4]

CAPTURE_DIR holds what the runtime wrote for RECOMP_CAPTURE=VA (see
src/kernel/recomp_capture.c): the guest state just before a call to VA
(pre_*) and just after the lifted function returned (post_*). This maps the
pre state into Unicorn, runs the XBE's own code for VA until it returns, and
compares the result with post: every differing dword of memory and every
differing register is a place where the lifted code and the original
disagree.

Float noise is expected -- the lifted x87 code computes in double where the
console rounds to the precision the control word asks for -- so dwords that
both read as finite floats within a relative tolerance are not reported. The
stack below the entry esp is the callee's dead frame and is not compared.
Calls into the kernel (thunks at 0xFE000000+) cannot be replayed; the run
stops there and says so.
"""

import argparse
import struct
import sys

import numpy as np
import unicorn as uc
from unicorn import x86_const as x

LOW_SIZE = 64 << 20
CONTIG_BASE = 0x80000000
CONTIG_SIZE = 128 << 20


def read_regs(path):
    regs = {}
    for line in open(path):
        k, *v = line.split()
        regs[k] = v if len(v) > 1 else v[0]
    return regs


def xbe_sections(xbe):
    d = open(xbe, 'rb').read()
    base = struct.unpack_from('<I', d, 0x104)[0]
    n = struct.unpack_from('<I', d, 0x11C)[0]
    sh = struct.unpack_from('<I', d, 0x120)[0] - base
    out = []
    for i in range(n):
        fl, va, vs, ra, rs = struct.unpack_from('<IIIII', d, sh + i * 56)
        out.append((va, vs, d[ra:ra + rs]))
    return out


def setup_gdt(mu, fs_base):
    """Flat code/data segments and an fs segment based at the TIB."""
    gdt_addr = 0xFFF00000
    mu.mem_map(gdt_addr, 0x1000)

    def desc(base, limit, access, flags):
        return struct.pack('<HHBBBB', limit & 0xFFFF, base & 0xFFFF,
                           (base >> 16) & 0xFF, access,
                           ((limit >> 16) & 0xF) | (flags << 4),
                           (base >> 24) & 0xFF)
    gdt = (b'\0' * 8 +
           desc(0, 0xFFFFF, 0x9A, 0xC) +      # 1: code
           desc(0, 0xFFFFF, 0x92, 0xC) +      # 2: data
           desc(fs_base, 0xFFFFF, 0x92, 0x4))  # 3: fs (byte granular)
    mu.mem_write(gdt_addr, gdt)
    mu.reg_write(x.UC_X86_REG_GDTR, (0, gdt_addr, len(gdt) - 1, 0))
    mu.reg_write(x.UC_X86_REG_CS, 1 << 3)
    mu.reg_write(x.UC_X86_REG_DS, 2 << 3)
    mu.reg_write(x.UC_X86_REG_ES, 2 << 3)
    mu.reg_write(x.UC_X86_REG_SS, 2 << 3)
    mu.reg_write(x.UC_X86_REG_FS, 3 << 3)


KERNEL_BASE = 0xFE000000


def kernel_stubs(xbe, low):
    """Thunk slot -> (ordinal, name, stdcall arg bytes), from the XBE's thunk
    table and the bridge's arg-size switch. The replay cannot run the kernel;
    a call returns 0 after popping its arguments, and is reported."""
    import os
    import re
    d = open(xbe, 'rb').read()
    base = struct.unpack_from('<I', d, 0x104)[0]
    # The runtime rewrote slot i as KERNEL_BASE + i*4: find the table that
    # way in the snapshot, then read the ordinals from the same VA in the XBE.
    thunk = bytes(low).find(struct.pack('<II', KERNEL_BASE, KERNEL_BASE + 4))
    secs = xbe_sections(xbe)

    def rd(a):
        for v, vs, raw in secs:
            if v <= a < v + len(raw):
                return struct.unpack_from('<I', raw, a - v)[0]
        return 0
    src = os.path.join(os.path.dirname(__file__), '../../src/kernel/kernel_bridge.c')
    sizes = {}
    for m in re.finditer(r'case (\d+): return +(\d+);\s*/\* (\w+)', open(src).read()):
        sizes[int(m.group(1))] = (int(m.group(2)), m.group(3))
    out = {}
    for i in range(512):
        e = rd(thunk + i * 4)
        if not e & 0x80000000:
            break
        o = e & 0xFFFF
        n, name = sizes.get(o, (0, f'ordinal{o}'))
        out[KERNEL_BASE + i * 4] = (o, name, n)
    return out


def run(xbe, cap, va, tol, watch=None, starts=None, ignore=(), x87_depth=0):
    pre = read_regs(f'{cap}/pre_regs.txt')
    post = read_regs(f'{cap}/post_regs.txt')
    low = bytearray(open(f'{cap}/pre_low.bin', 'rb').read())
    contig = bytearray(open(f'{cap}/pre_contig.bin', 'rb').read())

    mu = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
    mu.mem_map(0, LOW_SIZE)
    mu.mem_write(0, bytes(low))
    mu.mem_map(CONTIG_BASE, CONTIG_SIZE)
    mu.mem_write(CONTIG_BASE, bytes(contig))
    # The code comes from the snapshot: the loaded image, with the kernel
    # import table the runtime filled in (overwriting it with the XBE file
    # made every thunk slot differ).
    setup_gdt(mu, int(pre['fs_base']))

    for name, reg in (('eax', x.UC_X86_REG_EAX), ('ecx', x.UC_X86_REG_ECX),
                      ('edx', x.UC_X86_REG_EDX), ('ebx', x.UC_X86_REG_EBX),
                      ('esp', x.UC_X86_REG_ESP), ('esi', x.UC_X86_REG_ESI),
                      ('edi', x.UC_X86_REG_EDI), ('ebp', x.UC_X86_REG_EBP)):
        mu.reg_write(reg, int(pre[name]) & 0xFFFFFFFF)
    eflags = 0x202 | (0x400 if int(pre['df']) else 0)
    mu.reg_write(x.UC_X86_REG_EFLAGS, eflags)
    mu.reg_write(x.UC_X86_REG_FPCW, int(pre['fp_cw']) or 0x027F)
    if x87_depth:
        # The runtime keeps the x87 stack as doubles with a top index and no
        # tags; the caller says how many slots hold live values (a CRT
        # helper like _CIacos takes its argument in st(0)).
        from tools.recomp.insn_fuzz import d2ext
        top = int(pre['fp_top']) & 7
        tag = 0xFFFF
        for p in range(8):
            mu.reg_write(x.UC_X86_REG_FP0 + p, d2ext(float(pre[f'fp{p}'])))
        for i in range(x87_depth):
            tag &= ~(3 << (2 * ((top + i) & 7)))
        mu.reg_write(x.UC_X86_REG_FPSW, top << 11)
        mu.reg_write(x.UC_X86_REG_FPTAG, tag)
    for i in range(8):
        a, b, c, d = (int(v) for v in pre[f'xmm{i}'])
        mu.reg_write(x.UC_X86_REG_XMM0 + i, a | b << 32 | c << 64 | d << 96)

    # Device registers (NV2A at 0xFD000000, APU at 0xFE800000) as plain
    # zeroed memory: a replay that pokes the GPU carries on instead of
    # stopping, and these pages are not compared.
    mu.mem_map(0xFD000000, 0x01000000)
    mu.mem_map(0xFE800000, 0x00080000)
    stubs = kernel_stubs(xbe, low)
    mu.mem_map(KERNEL_BASE, 0x1000)
    mu.mem_write(KERNEL_BASE, b'\xC3' * 0x1000)
    kcalls = []

    def on_kernel(mu_, addr, size, data):
        o, name, n = stubs.get(addr, (0, f'slot@{addr:08X}', 0))
        esp = mu_.reg_read(x.UC_X86_REG_ESP)
        r = struct.unpack('<I', mu_.mem_read(esp, 4))[0]
        kcalls.append(f'{name}<-{r:08X}')
        mu_.reg_write(x.UC_X86_REG_EAX, 0)
        mu_.mem_write(esp + n, struct.pack('<I', r))
        mu_.reg_write(x.UC_X86_REG_ESP, esp + n)
    mu.hook_add(uc.UC_HOOK_CODE, on_kernel, begin=KERNEL_BASE, end=KERNEL_BASE + 0xFFF)

    esp0 = int(pre['esp'])
    ret = struct.unpack_from('<I', low, esp0)[0]
    stop = {}

    def on_unmapped(mu_, access, addr, size, value, data):
        stop['why'] = f'unmapped {"write" if access in (uc.UC_MEM_WRITE_UNMAPPED,) else "access"} at 0x{addr:08X}'
        return False
    mu.hook_add(uc.UC_HOOK_MEM_UNMAPPED, on_unmapped)

    calls = []
    if starts:
        def on_block(mu_, addr, size, data):
            if addr in starts:
                calls.append(addr)
        mu.hook_add(uc.UC_HOOK_BLOCK, on_block)

    if watch:
        def on_write(mu_, access, addr, size, value, data):
            if addr <= watch < addr + size:
                eip = mu_.reg_read(x.UC_X86_REG_EIP)
                print(f'  write 0x{watch:08X} by insn 0x{eip:08X} value 0x{value & 0xFFFFFFFF:08X}'
                      f' ({struct.unpack("<f", struct.pack("<I", value & 0xFFFFFFFF))[0]:g})'
                      f'  esi=0x{mu_.reg_read(x.UC_X86_REG_ESI):08X} edi=0x{mu_.reg_read(x.UC_X86_REG_EDI):08X}'
                      f' eax=0x{mu_.reg_read(x.UC_X86_REG_EAX):08X} ecx=0x{mu_.reg_read(x.UC_X86_REG_ECX):08X}'
                      f' edx=0x{mu_.reg_read(x.UC_X86_REG_EDX):08X} esp=0x{mu_.reg_read(x.UC_X86_REG_ESP):08X}'
                      f' stack={[hex(v) for v in struct.unpack("<8I", mu_.mem_read(mu_.reg_read(x.UC_X86_REG_ESP), 32))]}')
        mu.hook_add(uc.UC_HOOK_MEM_WRITE, on_write)

    try:
        mu.emu_start(va, ret, count=200_000_000)
    except uc.UcError as e:
        eip = mu.reg_read(x.UC_X86_REG_EIP)
        print(f'replay stopped at 0x{eip:08X}: {e} {stop.get("why", "")}')
        if kcalls:
            print('kernel calls before: ' + ' '.join(kcalls[-10:]))
        return 2

    if kcalls:
        from collections import Counter
        print('kernel calls stubbed (return 0): ' +
              ', '.join(f'{k} x{c}' for k, c in Counter(kcalls).items()))
    eip = mu.reg_read(x.UC_X86_REG_EIP)
    if eip != ret:
        print(f'replay did not return (eip 0x{eip:08X})')
        return 2

    if starts:
        import os
        lifted = []
        path = f'{cap}/calls.txt'
        if os.path.exists(path):
            lifted = [int(l, 16) for l in open(path) if l.strip()]
        import difflib
        orig = [c for c in calls if c != va and c not in ignore]
        lifted = [c for c in lifted if c != va and c not in ignore]
        print(f'calls: original {len(orig)}, lifted {len(lifted)}')
        sm = difflib.SequenceMatcher(a=orig, b=lifted, autojunk=False)
        shown = 0
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == 'equal' or shown >= 40:
                continue
            shown += 1
            ctx = ' '.join(f'{c:08X}' for c in orig[max(0, i1 - 3):i1])
            print(f'  {tag} at original #{i1}/lifted #{j1} (after {ctx}):')
            print('    original: ' + ' '.join(f'{c:08X}' for c in orig[i1:min(i2, i1 + 12)]))
            print('    lifted:   ' + ' '.join(f'{c:08X}' for c in lifted[j1:min(j2, j1 + 12)]))

    # Registers.
    bad = 0
    if x87_depth:
        from tools.recomp.insn_fuzz import ext2d
        ptop = int(post['fp_top']) & 7
        utop = (mu.reg_read(x.UC_X86_REG_FPSW) >> 11) & 7
        if ptop != utop:
            print(f'x87 top: original {utop}  lifted {ptop}')
            bad += 1
        for i in range(2):
            p = (utop + i) & 7
            o = ext2d(mu.reg_read(x.UC_X86_REG_FP0 + p)); l = float(post[f'fp{p}'])
            if not (o == l or abs(o - l) <= 1e-9 * max(1.0, abs(o)) or (o != o and l != l)):
                print(f'st({i}): original {o!r}  lifted {l!r}')
                bad += 1
    for name, reg in (('eax', x.UC_X86_REG_EAX), ('edx', x.UC_X86_REG_EDX),
                      ('ebx', x.UC_X86_REG_EBX), ('esi', x.UC_X86_REG_ESI),
                      ('edi', x.UC_X86_REG_EDI), ('esp', x.UC_X86_REG_ESP)):
        a = mu.reg_read(reg) & 0xFFFFFFFF
        b = int(post[name]) & 0xFFFFFFFF
        if a != b:
            print(f'register {name}: original 0x{a:08X}  lifted 0x{b:08X}')
            bad += 1

    # Memory, as dwords.
    other = 0

    def compare(region_base, orig, lifted, label, pre_bytes):
        """Report where the original wrote and the lifted code disagrees.
        Addresses only the live run changed are counted apart: on a running
        title another thread or an interrupt can write memory during the
        captured call, and that is not the function's doing."""
        nonlocal bad, other
        o = np.frombuffer(orig, dtype='<u4')
        l = np.frombuffer(lifted, dtype='<u4')
        p = np.frombuffer(pre_bytes, dtype='<u4')
        diff = o != l
        only_live = diff & (o == p)
        other += int(np.count_nonzero(only_live))
        idx = np.nonzero(diff & ~only_live)[0]
        shown = 0
        for i in idx:
            addr = region_base + int(i) * 4
            if region_base == 0 and esp0 - 0x40000 <= addr < esp0:
                continue                      # the callee's dead frame
            fo, fl = np.float32(o[i:i+1].view('<f4')[0]), np.float32(l[i:i+1].view('<f4')[0])
            if np.isfinite(fo) and np.isfinite(fl) and \
                    abs(fo - fl) <= tol * max(1.0, abs(fo), abs(fl)):
                continue                      # float rounding
            bad += 1
            if shown < 40:
                print(f'{label} 0x{addr:08X}: original 0x{int(o[i]):08X} ({fo:g})'
                      f'  lifted 0x{int(l[i]):08X} ({fl:g})')
                shown += 1
        if len(idx) and shown >= 40:
            print(f'{label}: ... more')

    compare(0, mu.mem_read(0, LOW_SIZE), open(f'{cap}/post_low.bin', 'rb').read(), 'low',
            bytes(low))
    compare(CONTIG_BASE, mu.mem_read(CONTIG_BASE, CONTIG_SIZE),
            open(f'{cap}/post_contig.bin', 'rb').read(), 'contig', bytes(contig))
    if other:
        print(f'({other} dword(s) changed only in the live run: other threads/interrupts, '
              f'or a lifted write the original does not make)')
    print('MATCH' if not bad else f'{bad} difference(s)')
    return 1 if bad else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('xbe')
    ap.add_argument('capture_dir')
    ap.add_argument('va', type=lambda s: int(s, 0))
    ap.add_argument('--tol', type=float, default=1e-4)
    ap.add_argument('--ignore-calls', default='', help='comma-separated VAs left out of the call comparison (labels the tracer mistakes for entries)')
    ap.add_argument('--functions', default=None,
                    help='file of known function starts (hex, one per line): compare call sequences with calls.txt')
    ap.add_argument('--x87-depth', type=int, default=0,
                    help='load the captured x87 stack, this many slots live, and compare st(0..1)')
    ap.add_argument('--watch', type=lambda s: int(s, 0), default=None,
                    help='report every write the original code makes to this address')
    a = ap.parse_args()
    starts = None
    if a.functions:
        starts = {int(l, 16) for l in open(a.functions) if l.strip()}
    ignore = {int(v, 16) for v in a.ignore_calls.split(",") if v}
    sys.exit(run(a.xbe, a.capture_dir, a.va, a.tol, a.watch, starts, ignore, a.x87_depth))


if __name__ == '__main__':
    main()
