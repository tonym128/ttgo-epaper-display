#!/usr/bin/env python3
"""
build_web_binaries.py
Compiles all PlatformIO firmware projects in the monorepo and merges the bootloader,
partitions, and app firmware into standalone 0x0 offset binaries for web flashing via ESP Web Tools.
"""

import os
import subprocess
import sys

PROJECTS = [
    ("multi_display", "multi_display_merged.bin"),
    ("weather", "weather_merged.bin"),
    ("daily_calendar", "daily_calendar_merged.bin"),
    ("picture_frame", "picture_frame_merged.bin"),
    ("smart_badge", "smart_badge_merged.bin"),
    ("news_reader", "news_reader_merged.bin"),
]

def find_esptool():
    import shutil
    tool = shutil.which("esptool.py")
    if tool:
        return tool
    home = os.path.expanduser("~")
    candidate = os.path.join(home, ".platformio/packages/tool-esptoolpy/esptool.py")
    if os.path.isfile(candidate):
        return candidate
    return "esptool.py"

def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    docs_bin = os.path.join(repo_root, "docs", "bin")
    os.makedirs(docs_bin, exist_ok=True)
    esptool = find_esptool()

    print(f"=== E-Display Web Binary Generator ===")
    print(f"Repo Root: {repo_root}")
    print(f"Output:    {docs_bin}")
    print(f"Esptool:   {esptool}\n")

    # Temporarily hide .env to guarantee clean, sanitized binaries for public distribution
    priv_env = os.path.join(repo_root, ".env")
    priv_env_bak = os.path.join(repo_root, ".env.build_bak")
    has_priv_env = os.path.isfile(priv_env)
    if has_priv_env:
        print("[Security] Stashing local .env to compile clean public binaries...")
        os.rename(priv_env, priv_env_bak)

    try:
        for proj_dir, out_name in PROJECTS:
            proj_path = os.path.join(repo_root, proj_dir)
            print(f"--> Cleaning {proj_dir}...")
            subprocess.run(["pio", "run", "-t", "clean", "-d", proj_path], cwd=repo_root, stdout=subprocess.DEVNULL)

            print(f"--> Building {proj_dir}...")
            res = subprocess.run(["pio", "run", "-d", proj_path], cwd=repo_root)
            if res.returncode != 0:
                print(f"[ERROR] Failed to compile {proj_dir}!")
                sys.exit(1)

            build_dir = os.path.join(proj_path, ".pio", "build", "esp32dev")
            bootloader = os.path.join(build_dir, "bootloader.bin")
            partitions = os.path.join(build_dir, "partitions.bin")
            firmware = os.path.join(build_dir, "firmware.bin")
            merged_out = os.path.join(docs_bin, out_name)

            print(f"--> Merging binary for {proj_dir} -> {out_name}...")
            merge_cmd = [
                sys.executable, esptool,
                "--chip", "esp32", "merge_bin",
                "-o", merged_out,
                "--flash_mode", "dio",
                "--flash_freq", "40m",
                "--flash_size", "4MB",
                "0x1000", bootloader,
                "0x8000", partitions,
                "0x10000", firmware
            ]
            res_merge = subprocess.run(merge_cmd, cwd=repo_root)
            if res_merge.returncode != 0:
                print(f"[ERROR] Failed to merge binary for {proj_dir}!")
                sys.exit(1)
            print(f"    [OK] Created {merged_out} ({os.path.getsize(merged_out)} bytes)\n")

        print("=== All Web Binaries Generated Successfully! ===")
    finally:
        if has_priv_env and os.path.isfile(priv_env_bak):
            print("[Security] Restoring local .env...")
            os.rename(priv_env_bak, priv_env)

if __name__ == "__main__":
    main()
