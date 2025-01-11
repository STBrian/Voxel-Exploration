import os, subprocess
from pathlib import Path

currentPath = Path(os.path.realpath(__file__)).parent
rootFolder = currentPath.parent
os.chdir(currentPath)
relativePath = currentPath.relative_to

for file in currentPath.rglob("*.png"):
    relativePath = file.relative_to(rootFolder)
    outPath = Path(f"{rootFolder.joinpath("romfs", relativePath.parent, relativePath.stem)}.t3x")
    if not outPath.parent.exists():
        os.makedirs(outPath.parent)

    build = False
    if not outPath.exists() or (outPath.exists() and file.stat().st_mtime > outPath.stat().st_mtime):
        print(relativePath)
        convertTask = subprocess.run(
            [
                "tex3ds",
                "-o", outPath,
                file
            ]
        )