// game_module.h — load recompiled cart and BIOS code from game modules built
// on the player's machine (src/module/gbarecomp_module_abi.h,
// docs/GAME_MODULES.md).
#pragma once

#include <string>

namespace gbarecomp {

enum class GameModuleKind { Cart, Bios };

// Loads the module at `path` (a .dll or .so), checks that it holds `kind`
// code made from the file whose SHA-1 is `expected_sha1` (lower-case hex), and
// installs its dispatch table, copied-RAM templates and symbols in place of
// the executable's own. Call before the game starts running, at most once per
// kind; a loaded module stays loaded for the life of the process.
bool load_game_module(const std::string& path, GameModuleKind kind,
                      const std::string& expected_sha1, std::string* error);

// True once a module of this kind is installed.
bool game_module_loaded(GameModuleKind kind);

}  // namespace gbarecomp
