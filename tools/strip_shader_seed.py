#!/usr/bin/env python3
"""Make a shader seed that carries none of the game's code.

    strip_shader_seed.py <recorded seed dir> <default.xbe> <out dir>

A recorded seed (a shader cache's states.bin and pipelines.bin) holds the
game's vertex programs as they were loaded to the GPU. Each one is in
default.xbe as is: here it is replaced with where it is there (offset,
length and hash), and the renderer reads it back from the player's own copy
(NV2A_SHADER_SEED_XBE). pipelines.bin holds no program and is copied.

The offsets below are those of ShaderState in this build (state size 2768);
another layout is refused.
"""
import shutil
import struct
import sys

import xxhash

STATES_MAGIC = 0x53543244
XBE_REF_MAGIC = 0x52454258          # "XBER"
STATE_SIZE = 2768
PROG_OFF = 208                      # vsh.programmable.program_data
PROG_SIZE = 136 * 16
LEN_OFF = 2384                      # vsh.programmable.program_length


def main():
    src, xbe_path, out = sys.argv[1:4]
    xbe = open(xbe_path, 'rb').read()
    data = open(f'{src}/states.bin', 'rb').read()
    magic, _key_size, state_size = struct.unpack_from('<III', data)
    if magic != STATES_MAGIC or state_size != STATE_SIZE:
        sys.exit('states.bin: not the layout this tool knows')

    outb = bytearray(data[:12])
    n = (len(data) - 12) // state_size
    refs = missing = 0
    for i in range(n):
        st = bytearray(data[12 + i * state_size:12 + (i + 1) * state_size])
        length = struct.unpack_from('<i', st, LEN_OFF)[0]
        if length > 0:
            prog = bytes(st[PROG_OFF:PROG_OFF + length * 16])
            at = xbe.find(prog)
            if at < 0:
                missing += 1
                continue        # not in the executable: leave the state out
            st[PROG_OFF:PROG_OFF + PROG_SIZE] = bytes(PROG_SIZE)
            h = xxhash.xxh3_64_intdigest(prog)
            struct.pack_into('<IIIII', st, PROG_OFF, XBE_REF_MAGIC, at,
                             len(prog), h & 0xffffffff, h >> 32)
            refs += 1
        outb += st
    open(f'{out}/states.bin', 'wb').write(outb)
    shutil.copyfile(f'{src}/pipelines.bin', f'{out}/pipelines.bin')
    print(f'{n} states: {refs} programs referenced, {missing} left out')


if __name__ == '__main__':
    main()
