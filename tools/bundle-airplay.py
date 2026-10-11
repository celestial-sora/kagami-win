"""Resolve non-system PE imports for the relocatable Windows AirPlay runtime."""
from pathlib import Path
import re, shutil, subprocess, sys
root, runtime = map(Path, sys.argv[1:])
queue = list(root.rglob('*.dll')) + list(root.rglob('*.exe'))
seen = set()
while queue:
    file = queue.pop()
    if file.name.lower() in seen:
        continue
    seen.add(file.name.lower())
    result = subprocess.run(['objdump', '-p', str(file)], capture_output=True, text=True, check=True)
    for name in re.findall(r'DLL Name:\s*(\S+)', result.stdout):
        source = runtime / name
        if source.is_file() and name.lower() not in seen:
            target = root / 'bin' / name
            shutil.copy2(source, target)
            queue.append(target)
