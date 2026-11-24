# PlatformIO post-script: make VS Code IntelliSense understand SDCC (STM8).
#
# cpptools can't query sdcc.exe for its built-in macros, so __SDCC is missing
# and the STM8 SPL headers fall into the IAR branch (intrinsics.h not found).
# These defines are added ONLY when PlatformIO dumps IDE data, never to the
# real build.
#
# Usage in platformio.ini:
#   extra_scripts = post:utils/pio_sdcc_intellisense.py   (post: the STM8 builder
#   replaces CPPDEFINES, so a pre: script would be wiped)

Import("env", "projenv")


def _is_ide_dump():
    try:
        return env.IsIntegrationDump()
    except AttributeError:
        return "idedata" in COMMAND_LINE_TARGETS  # noqa: F821


if _is_ide_dump():
    # idedata reads defines from projenv (already cloned before post-scripts)
    projenv.Append(
        CPPDEFINES=[
            "__SDCC",
            ("__SDCC_VERSION_MAJOR", 3),
            ("__SDCC_VERSION_MINOR", 9),
            ("__SDCC_VERSION_PATCH", 1),
            "__SDCC_stm8",
            # SDCC-only keywords -> nothing, so cpptools can parse them
            ("__interrupt(x)", ""),
            ("__trap", ""),
            ("__at(x)", ""),
            ("__naked", ""),
            ("__critical", ""),
            ("__code", ""),
            ("__data", ""),
            ("__near", ""),
            ("__far", ""),
            ("__sfr", "volatile unsigned char"),
            ("__asm__(x)", ""),
        ]
    )
