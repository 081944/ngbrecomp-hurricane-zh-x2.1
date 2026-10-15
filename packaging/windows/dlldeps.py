#!/usr/bin/env python3
"""Print the DLLs a Windows .exe needs that Windows itself does not ship, resolved
recursively against the given search dirs (one path per line).
usage: dlldeps.py OBJDUMP EXE DIR..."""
import os, subprocess, sys

objdump, exe, dirs = sys.argv[1], sys.argv[2], sys.argv[3:]

def imports(path):
    out = subprocess.run([objdump, "-p", path], capture_output=True, text=True).stdout
    return [l.split("DLL Name:")[1].strip() for l in out.splitlines() if "DLL Name:" in l]

def find(name):
    for d in dirs:
        for f in os.listdir(d):
            if f.lower() == name.lower():
                return os.path.join(d, f)
    return None

seen, todo, missing = {}, [exe], set()
while todo:
    for name in imports(todo.pop()):
        key = name.lower()
        if key in seen or key in missing:
            continue
        p = find(name)
        if p:
            seen[key] = p
            todo.append(p)
        else:
            missing.add(key)   # a system DLL (kernel32, d3d11, vulkan-1, ...)
for p in sorted(seen.values()):
    print(p)
print("system:", " ".join(sorted(missing)), file=sys.stderr)
