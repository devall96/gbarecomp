// game_module.cpp — see game_module.h.
#include "game_module.h"

#include "../module/gbarecomp_module_abi.h"
#include "native_program.h"
#include "runtime_arm.h"
#include "symbol_lookup.h"

#include <cstring>
#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

extern "C" void runtime_install_module_dispatch(int bios, const DispatchEntry* table,
                                                unsigned count);
extern "C" uint32_t runtime_native_ram_base_value(void);
extern "C" void runtime_exception_return(uint32_t new_pc);
extern "C" uint32_t runtime_current_pc(void);

#if defined(GBARECOMP_GAME_MODULE)
// A module build links no generated cart code; the loaded module supplies it.
extern "C" const DispatchEntry kDispatchTable[] = {{0u, 0u, 0u, nullptr}};
extern "C" const unsigned kDispatchTableLen = 0;
#endif

namespace gbarecomp {
namespace {

bool s_loaded[2] = {false, false};

GbaModuleHost make_host() {
    GbaModuleHost h{};
    h.abi_version = GBARECOMP_MODULE_ABI_VERSION;
    h.size = sizeof(GbaModuleHost);
    h.cpu = &g_cpu;
    h.fn_entry_hook = &g_runtime_fn_entry_hook;
    h.insn_trace = &g_runtime_insn_trace;
    h.resume_pc = &g_runtime_resume_pc;
    h.trace_enabled = &g_runtime_trace_enabled;
    h.waits = &g_runtime_waits;
    h.thumb_alu_imm_override = &g_runtime_thumb_alu_imm_override;
    h.native_ram_base = runtime_native_ram_base_value;

    h.arm_cond_passes = arm_cond_passes;
    h.arm_shift_lsl = arm_shift_lsl;
    h.arm_shift_lsr = arm_shift_lsr;
    h.arm_shift_asr = arm_shift_asr;
    h.arm_shift_ror = arm_shift_ror;
    h.arm_set_nz = arm_set_nz;
    h.arm_set_nzc_logic = arm_set_nzc_logic;
    h.arm_set_nzcv_add = arm_set_nzcv_add;
    h.arm_set_nzcv_adc = arm_set_nzcv_adc;
    h.arm_set_nzcv_sub = arm_set_nzcv_sub;
    h.arm_set_nzcv_sbc = arm_set_nzcv_sbc;

    h.bus_read_u32 = bus_read_u32;
    h.bus_read_u16 = bus_read_u16;
    h.bus_read_u8 = bus_read_u8;
    h.bus_write_u32 = bus_write_u32;
    h.bus_write_u16 = bus_write_u16;
    h.bus_write_u8 = bus_write_u8;
    h.runtime_mem_cycles = runtime_mem_cycles;
    h.runtime_mul_cycles = runtime_mul_cycles;
    h.runtime_prefetch_stall_delta = runtime_prefetch_stall_delta;
    h.runtime_exception_return_refill = runtime_exception_return_refill;
    h.runtime_tick = runtime_tick;
    h.runtime_should_yield = runtime_should_yield;
    h.runtime_idle_backedge = runtime_idle_backedge;

    h.runtime_dispatch = runtime_dispatch;
    h.runtime_dispatch_with_exchange = runtime_dispatch_with_exchange;
    h.runtime_dispatch_miss = runtime_dispatch_miss;
    h.runtime_call_push_return = runtime_call_push_return;
    h.runtime_call_should_return = runtime_call_should_return;
    h.runtime_call_cancel_return = runtime_call_cancel_return;
    h.runtime_has_static_entry = runtime_has_static_entry;
    h.runtime_bridge_interpret = runtime_bridge_interpret;
    h.runtime_force_interp_step = runtime_force_interp_step;
    h.gba_mod_function_entry = gba_mod_function_entry;

    h.runtime_swi = runtime_swi;
    h.runtime_irq = runtime_irq;
    h.runtime_exception_return = runtime_exception_return;
    h.runtime_mrs_cpsr = runtime_mrs_cpsr;
    h.runtime_mrs_spsr = runtime_mrs_spsr;
    h.runtime_msr_cpsr = runtime_msr_cpsr;
    h.runtime_msr_spsr = runtime_msr_spsr;
    h.runtime_read_user_reg = runtime_read_user_reg;
    h.runtime_write_user_reg = runtime_write_user_reg;

    h.runtime_native_ram_instruction_matches = runtime_native_ram_instruction_matches;
    h.runtime_native_ram_instruction_matches_masked = runtime_native_ram_instruction_matches_masked;
    h.runtime_native_arm_immediate = runtime_native_arm_immediate;

    h.runtime_trace_event = runtime_trace_event;
    h.runtime_insn_fp = runtime_insn_fp;
    h.runtime_current_pc = runtime_current_pc;

    h.native_ram_code_source = &g_runtime_native_ram_code_source;
    h.ram_code_view = &g_runtime_ram_code_view;

    h.runtime_unimplemented_op = runtime_unimplemented_op;
    h.runtime_note_interpreted_exception_return = runtime_note_interpreted_exception_return;
    h.runtime_restore_cpsr_from_spsr = runtime_restore_cpsr_from_spsr;
    return h;
}

void* open_library(const std::string& path, std::string* error) {
#if defined(_WIN32)
    const std::wstring wide = std::filesystem::u8path(path).wstring();
    // Look for the module's own dependencies next to it, not in the cwd.
    HMODULE m = LoadLibraryExW(wide.c_str(), nullptr,
                               LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!m) *error = "could not load " + path + " (error " + std::to_string(GetLastError()) + ")";
    return reinterpret_cast<void*>(m);
#else
    void* m = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!m) *error = std::string("could not load ") + path + ": " + dlerror();
    return m;
#endif
}

void* find_symbol(void* library, const char* name) {
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(library), name));
#else
    return dlsym(library, name);
#endif
}

void close_library(void* library) {
#if defined(_WIN32)
    FreeLibrary(reinterpret_cast<HMODULE>(library));
#else
    dlclose(library);
#endif
}

}  // namespace

bool game_module_loaded(GameModuleKind kind) {
    return s_loaded[kind == GameModuleKind::Bios ? 1 : 0];
}

bool load_game_module(const std::string& path, GameModuleKind kind,
                      const std::string& expected_sha1, std::string* error) {
    std::string scratch;
    std::string& err = error ? *error : scratch;
    const int bios = kind == GameModuleKind::Bios ? 1 : 0;
    if (s_loaded[bios]) {
        err = "a module of this kind is already loaded";
        return false;
    }
    void* library = open_library(path, &err);
    if (!library) return false;
    auto init = reinterpret_cast<GbaModuleInitFn>(find_symbol(library, GBARECOMP_MODULE_INIT_SYMBOL));
    if (!init) {
        close_library(library);
        err = path + " is not a gbarecomp game module";
        return false;
    }
    const GbaModuleHost host = make_host();
    GbaModuleTables tables{};
    if (!init(&host, &tables) || tables.abi_version != GBARECOMP_MODULE_ABI_VERSION ||
        tables.size != sizeof(GbaModuleTables)) {
        close_library(library);
        err = path + " was built for a different TempleFrog version";
        return false;
    }
    const uint32_t want = bios ? kGbaModuleBios : kGbaModuleCart;
    if (tables.kind != want) {
        close_library(library);
        err = path + (bios ? " holds no BIOS code" : " holds no cart code");
        return false;
    }
    if (!tables.source_sha1 || expected_sha1 != tables.source_sha1) {
        close_library(library);
        err = path + " was made from a different " + (bios ? "BIOS" : "ROM");
        return false;
    }
    if (!tables.dispatch || tables.dispatch_count == 0) {
        close_library(library);
        err = path + " has no code";
        return false;
    }

    runtime_install_module_dispatch(bios, static_cast<const DispatchEntry*>(tables.dispatch),
                                    tables.dispatch_count);
    if (!bios && tables.ram_templates) {
        runtime_register_native_ram_templates(
            static_cast<const RuntimeNativeRamTemplate*>(tables.ram_templates),
            tables.ram_template_count,
            static_cast<const RuntimeNativeRamEntry*>(tables.ram_entries), tables.ram_entry_count);
    }
    if (tables.symbols) {
        const auto* symbols = static_cast<const GbaSymbol*>(tables.symbols);
        if (bios) gba_symbol_register_bios(symbols, tables.symbol_count);
        else gba_symbol_register_cart(symbols, tables.symbol_count);
    }
    s_loaded[bios] = true;
    return true;
}

}  // namespace gbarecomp
