# Game modules: releases without the game's code

Code that `gba_recompile` makes from a ROM or BIOS is that game's (or the
console maker's) program in another form, and the generated tables carry raw
bytes of it too. A build that links `generated/` therefore carries the game,
however the ROM itself is handled. Game modules keep it out of published builds:

- The release ships the runtime, the launcher and a **game kit**: `gba_recompile`,
  the module SDK (`src/module/sdk`, `src/module/gbarecomp_module_abi.h`, and the
  few headers they use) and a C++ compiler.
- On the player's machine, once their ROM and BIOS are verified, the kit
  recompiles each into C++ and compiles it into a shared library: the **cart
  module** and the **BIOS module**.
- The runtime loads them at start (`src/runtime/game_module.cpp`) and uses their
  dispatch tables, copied-RAM templates and symbols in place of the ones a
  static build links in.

The generated sources compile unchanged. A module build puts `src/module/sdk`
first on the include path, so their `#include "runtime_arm.h"` reaches the SDK
copy, which presents the host's runtime under the same names: host globals
through pointers (`g_cpu` is `(*gbarecomp_module_host.cpu)`), host functions as
inline forwards through the `GbaModuleHost` table the host passes to
`gbarecomp_module_init`. A module has no unresolved host symbols, so the same
code builds with any compiler for Windows (DLL) and Linux (`.so`), and the
calls cost what calls through an import table cost.

## Building a game with modules

1. Configure the runtime with `GBARECOMP_GAME_MODULE=ON`. The executable then
   links an empty cart table and the placeholder BIOS table, and never the
   generated BIOS, even when `generated_bios` is present.
2. Set `RunOptions::prepare_native_code`. `run_game` calls it once the ROM and
   BIOS are resolved and verified, before anything runs, with their paths and
   SHA-1s. The game builds its modules if needed (showing progress) and loads
   them with `load_game_module()`, then returns 0, or an exit code to end with.
3. Build a module by running `gba_recompile` on the player's file into a work
   folder, then compiling every `.cpp` there plus `src/module/sdk/module_glue.cpp`
   (with `-DGBARECOMP_MODULE_CART` or `-DGBARECOMP_MODULE_BIOS` and
   `-DGBARECOMP_MODULE_SOURCE_SHA1="<sha1>"`) and linking them as a shared library.
   Use the optimisation the static build uses (`-O1` for the `recompiled*`
   shards). On ELF, compile with `-fPIC -fvisibility=hidden`; on the BIOS module
   define `_start=gbarecomp_bios_reset_start`. Delete the work folder after: it
   holds the generated sources.
4. `load_game_module()` refuses a module whose ABI, kind or source SHA-1 does
   not match. When the BIOS module is loaded, the runtime runs the recompiled
   BIOS (LLE) as a static build with `generated_bios` would; without it, BIOS
   HLE stands in, as in any build without the recompiled BIOS.

TempleFrog (`src/game_build.cpp`, `src/main.cpp`) is the reference: it bundles
Zig's clang as the compiler, builds on first start behind a progress window or
in RecompLauncher's `prepare` step, and stamps each module with the kit's id so
a new recompiler rebuilds it once.

## Keeping the SDK in step

`src/module/sdk/runtime_arm.h` mirrors what generated code uses from
`src/armv4t/runtime_arm.h`. When the generator starts using a new runtime name,
add it to `GbaModuleHost` (at the end), to `make_host()` in
`src/runtime/game_module.cpp` and to the SDK header, and bump
`GBARECOMP_MODULE_ABI_VERSION` if a field moved. A module build that misses a
name fails to compile, on the developer's machine first, so a mismatch never
reaches a player silently.

## Checking it

- A module build and a static build of the same generator output must produce
  the same frames: compare `--frames N --dump-bmp` at several N. (The first
  module build differed because the runtime picked BIOS HLE whenever the BIOS
  was not linked; it now asks `game_module_loaded()`.)
- `self_heal_coverage=FULLY_STATIC dispatch_misses=0` and
  `bios_backend=LLE (recompiled BIOS)` in the log.
- Frame times against the static build (the game's own perf suite).
