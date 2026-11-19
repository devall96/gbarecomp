// gbarecomp_module_abi.h — the contract between a gbarecomp runtime and a game
// module: a shared library (.dll / .so) holding the recompiled cart or BIOS code.
//
// Why modules exist: code recompiled from a ROM or BIOS is the game's (or the
// console maker's) code in another form, so a published build must not carry
// it. A release ships the runtime alone; the player's own ROM and BIOS are
// recompiled and compiled into modules on their machine (docs/GAME_MODULES.md),
// and the runtime loads them at start.
//
// The module has no unresolved host symbols. Every runtime function and
// global the generated code touches is reached through the GbaModuleHost
// table the host hands to gbarecomp_module_init(); the module's copy of the
// table is one global, so a call costs what a call through an import table
// costs, and a register-file access what a GOT/.refptr access costs. The
// generated sources compile unchanged against src/module/sdk/runtime_arm.h,
// which presents this table under the names the static build uses.
//
// Both sides are C++ (the generated code is C++). Grow the tables only at the
// end, and bump GBARECOMP_MODULE_ABI_VERSION for any change that moves a field.
#pragma once

#include <stdint.h>

#include "runtime_arm_types.h"  // ArmCpuState, RuntimeWaitTable, RuntimeTraceEntry

#define GBARECOMP_MODULE_ABI_VERSION 1u
#define GBARECOMP_MODULE_INIT_SYMBOL "gbarecomp_module_init"

enum GbaModuleKind : uint32_t {
    kGbaModuleCart = 1u,
    kGbaModuleBios = 2u,
};

extern "C" {

struct GbaModuleHost {
    uint32_t abi_version;
    uint32_t size;  // sizeof(GbaModuleHost) on the host

    // ── Host globals the generated code reads and writes ──
    ArmCpuState* cpu;
    void (**fn_entry_hook)(uint32_t entry_pc);
    unsigned* insn_trace;
    uint32_t* resume_pc;
    unsigned* trace_enabled;
    RuntimeWaitTable* waits;
    // RuntimeThumbAluImmediateOverride* (see runtime_arm.h)
    int (**thumb_alu_imm_override)(uint32_t, uint32_t, uint32_t*);
    // g_runtime_native_ram_base is thread-local on the host; a module reads
    // the calling thread's value through this function.
    uint32_t (*native_ram_base)(void);

    // ── Condition, shifter, flags ──
    int (*arm_cond_passes)(unsigned cond);
    uint32_t (*arm_shift_lsl)(uint32_t v, uint32_t n, int set_carry);
    uint32_t (*arm_shift_lsr)(uint32_t v, uint32_t n, int set_carry);
    uint32_t (*arm_shift_asr)(uint32_t v, uint32_t n, int set_carry);
    uint32_t (*arm_shift_ror)(uint32_t v, uint32_t n, int set_carry);
    void (*arm_set_nz)(uint32_t result);
    void (*arm_set_nzc_logic)(uint32_t result, uint32_t shifter_carry);
    void (*arm_set_nzcv_add)(uint32_t a, uint32_t b, uint32_t result);
    void (*arm_set_nzcv_adc)(uint32_t a, uint32_t b, uint32_t c_in, uint32_t result);
    void (*arm_set_nzcv_sub)(uint32_t a, uint32_t b, uint32_t result);
    void (*arm_set_nzcv_sbc)(uint32_t a, uint32_t b, uint32_t c_in, uint32_t result);

    // ── Bus and timing ──
    uint32_t (*bus_read_u32)(uint32_t addr);
    uint16_t (*bus_read_u16)(uint32_t addr);
    uint8_t (*bus_read_u8)(uint32_t addr);
    void (*bus_write_u32)(uint32_t addr, uint32_t val);
    void (*bus_write_u16)(uint32_t addr, uint16_t val);
    void (*bus_write_u8)(uint32_t addr, uint8_t val);
    uint32_t (*runtime_mem_cycles)(uint32_t addr, uint32_t width, uint32_t sequential);
    uint32_t (*runtime_mul_cycles)(uint32_t rs_value, uint32_t signed_variant, uint32_t extra);
    uint32_t (*runtime_prefetch_stall_delta)(uint32_t wait, uint32_t pc, uint32_t thumb);
    uint32_t (*runtime_exception_return_refill)(uint32_t target_pc);
    void (*runtime_tick)(uint32_t cycles);
    bool (*runtime_should_yield)(void);
    void (*runtime_idle_backedge)(uint32_t header_pc);

    // ── Dispatch and calls ──
    void (*runtime_dispatch)(uint32_t target_pc);
    void (*runtime_dispatch_with_exchange)(uint32_t target_pc);
    void (*runtime_dispatch_miss)(uint32_t target_pc);
    void (*runtime_call_push_return)(uint32_t return_pc);
    int (*runtime_call_should_return)(uint32_t target_pc);
    void (*runtime_call_cancel_return)(uint32_t return_pc);
    int (*runtime_has_static_entry)(uint32_t pc, int thumb);
    int (*runtime_bridge_interpret)(uint32_t entry_pc, bool entry_thumb,
                                    uint32_t forced_stop_pc, uint64_t max_instrs);
    void (*runtime_force_interp_step)(void);
    int (*gba_mod_function_entry)(uint32_t entry_pc, int thumb, ArmCpuState* cpu);

    // ── Exceptions and status registers ──
    void (*runtime_swi)(uint32_t swi_imm);
    void (*runtime_irq)(uint32_t return_address);
    void (*runtime_exception_return)(uint32_t new_pc);
    uint32_t (*runtime_mrs_cpsr)(void);
    uint32_t (*runtime_mrs_spsr)(void);
    void (*runtime_msr_cpsr)(uint32_t value, uint32_t mask);
    void (*runtime_msr_spsr)(uint32_t value, uint32_t mask);
    uint32_t (*runtime_read_user_reg)(uint32_t reg);
    void (*runtime_write_user_reg)(uint32_t reg, uint32_t value);

    // ── Copied-RAM code ──
    int (*runtime_native_ram_instruction_matches)(uint32_t pc, uint32_t opcode, uint32_t size);
    int (*runtime_native_ram_instruction_matches_masked)(uint32_t pc, uint32_t opcode,
                                                         uint32_t size, uint32_t mask);
    uint32_t (*runtime_native_arm_immediate)(uint32_t pc);

    // ── Tracing ──
    void (*runtime_trace_event)(uint32_t kind, uint32_t pc, uint32_t addr,
                                uint32_t value, uint32_t aux);
    void (*runtime_insn_fp)(void);
    uint32_t (*runtime_current_pc)(void);

    // ── Copied-RAM code views (RuntimeNativeRamCodeView in runtime_arm.h) ──
    void* native_ram_code_source;  // RuntimeNativeRamCodeSource* (g_runtime_native_ram_code_source)
    void* ram_code_view;           // RuntimeRamCodeView* (g_runtime_ram_code_view)

    // ── Rare paths ──
    void (*runtime_unimplemented_op)(const char* op_name, uint32_t pc);
    void (*runtime_note_interpreted_exception_return)(uint32_t old_mode);
    void (*runtime_restore_cpsr_from_spsr)(void);
};

// What a module hands back. Pointers stay valid while the module is loaded.
struct GbaModuleTables {
    uint32_t abi_version;
    uint32_t size;  // sizeof(GbaModuleTables) in the module
    uint32_t kind;  // GbaModuleKind
    const char* source_sha1;  // SHA-1 of the ROM or BIOS the code was made from

    const void* dispatch;  // const DispatchEntry[dispatch_count]
    uint32_t dispatch_count;
    const void* ram_templates;  // const RuntimeNativeRamTemplate[ram_template_count]
    uint32_t ram_template_count;
    const void* ram_entries;  // const RuntimeNativeRamEntry[ram_entry_count]
    uint32_t ram_entry_count;
    const void* symbols;  // const GbaSymbol[symbol_count]
    uint32_t symbol_count;
};

// Exported by every module. Copies *host, fills *out and returns 1; returns 0
// (and leaves *out zeroed) when the host's ABI does not match.
typedef int (*GbaModuleInitFn)(const GbaModuleHost* host, GbaModuleTables* out);

}  // extern "C"
