import subprocess
import shutil
import os
import glob

SRC = "src"
DIST = "dist"
BIN = "avx-argb2gray.exe"

GCC = r"C:\mingw-w64\x86_64-8.1.0-release-win32-sjlj-rt_v6-rev0\mingw64\bin"

def run_cmd(cmd):
    print(cmd)
    step = subprocess.run(cmd)
    step.check_returncode()
    
def clear_dir(dir):
    try:
        shutil.rmtree(dir, ignore_errors=True)
    except FileNotFoundError:
        pass
    os.mkdir(dir)

def build_bin():
    cpp = glob.glob(f"{SRC}/**/*.cpp", recursive=True)
    dlls = glob.glob("*.dll")
    for dll in dlls:
        shutil.copyfile(dll, f"{DIST}/{dll}")
    run_cmd(f'{GCC}/g++ -s -O3 -fno-exceptions -march=i686 -m32 -mavx -mavx2 -masm=intel {" ".join(cpp)} -static-libgcc -static-libstdc++ -o {DIST}/{BIN}')

if __name__ == "__main__":
    clear_dir(DIST)
    build_bin()
