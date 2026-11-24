# PlatformIO pre-script: override the project src dir for a single env.
#
# Usage in platformio.ini:
#   extra_scripts = pre:utils/pio_src_dir.py
#   custom_src_dir = src/projects/<path-to-node>
#
# Needed for C-only frameworks (e.g. ST STM8 / Sduino) whose builder scans the
# whole $PROJECT_SRC_DIR for C++ files and aborts, ignoring build_src_filter.
import os

Import("env")

custom_src_dir = env.GetProjectOption("custom_src_dir", "")
if custom_src_dir:
    path = os.path.join(env.subst("$PROJECT_DIR"), custom_src_dir)
    env.Replace(PROJECT_SRC_DIR=os.path.normpath(path))
