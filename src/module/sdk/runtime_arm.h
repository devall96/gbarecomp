// runtime_arm.h for game modules (see ../gbarecomp_module_abi.h).
//
// A module build puts this directory ahead of src/armv4t on the include path,
// so the generated sources' `#include "runtime_arm.h"` lands here. It presents
// the host's runtime under the names src/armv4t/runtime_arm.h declares: host
// globals are reached through pointers in the module's copy of the host table
// (`g_cpu` is `(*gbarecomp_module_host.cpu)`), and host functions are
// inline forwards through that table. The generated code compiles unchanged.
//
// Keep it in step with src/armv4t/runtime_arm.h: a name the generator starts
// using must be added to GbaModuleHost and here. A module build that misses
// one fails to compile (tools/module_check, and every player build), so a
// mismatch cannot reach a player silently.
#pragma once

#include <stdint.h>

#include "gbarecomp_module_abi.h"

extern "C" {

// The module's copy of the host table (module_glue.cpp).
extern GbaModuleHost gbarecomp_module_host;

// ── Host globals ──
#define g_cpu (*gbarecomp_module_host.cpu)
#define g_runtime_fn_entry_hook (*gbarecomp_module_host.fn_entry_hook)
#define g_runtime_insn_trace (*gbarecomp_module_host.insn_trace)
#define g_runtime_resume_pc (*gbarecomp_module_host.resume_pc)
#define g_runtime_trace_enabled (*gbarecomp_module_host.trace_enabled)
#define g_runtime_waits (*gbarecomp_module_host.waits)
#define g_runtime_thumb_alu_imm_override (*gbarecomp_module_host.thumb_alu_imm_override)
#define g_runtime_native_ram_base (gbarecomp_module_host.native_ram_base())

typedef int (*RuntimeThumbAluImmediateOverride)(uint32_t instruction_pc,
                                                uint32_t original_value,
                                                uint32_t* out_value);

static inline uint32_t runtime_thumb_alu_immediate(uint32_t instruction_pc,
                                                   uint32_t original_value) {
    uint32_t overridden = original_value;
    if (g_runtime_thumb_alu_imm_override &&
        g_runtime_thumb_alu_imm_override(instruction_pc, original_value, &overridden)) {
        return overridden;
    }
    return original_value;
}

static inline uint32_t cpsr_n(void) { return (g_cpu.cpsr & CPSR_N_BIT) ? 1u : 0u; }
static inline uint32_t cpsr_z(void) { return (g_cpu.cpsr & CPSR_Z_BIT) ? 1u : 0u; }
static inline uint32_t cpsr_c(void) { return (g_cpu.cpsr & CPSR_C_BIT) ? 1u : 0u; }
static inline uint32_t cpsr_v(void) { return (g_cpu.cpsr & CPSR_V_BIT) ? 1u : 0u; }

static inline int runtime_trace_enabled(void) { return g_runtime_trace_enabled != 0u; }

#define GBARECOMP_MODULE_FORWARD(ret, name, params, args) \
    static inline ret name params { return gbarecomp_module_host.name args; }

// ── Condition, shifter, flags ──
GBARECOMP_MODULE_FORWARD(int, arm_cond_passes, (unsigned c), (c))
GBARECOMP_MODULE_FORWARD(uint32_t, arm_shift_lsl, (uint32_t v, uint32_t n, int s), (v, n, s))
GBARECOMP_MODULE_FORWARD(uint32_t, arm_shift_lsr, (uint32_t v, uint32_t n, int s), (v, n, s))
GBARECOMP_MODULE_FORWARD(uint32_t, arm_shift_asr, (uint32_t v, uint32_t n, int s), (v, n, s))
GBARECOMP_MODULE_FORWARD(uint32_t, arm_shift_ror, (uint32_t v, uint32_t n, int s), (v, n, s))
GBARECOMP_MODULE_FORWARD(void, arm_set_nz, (uint32_t r), (r))
GBARECOMP_MODULE_FORWARD(void, arm_set_nzc_logic, (uint32_t r, uint32_t c), (r, c))
GBARECOMP_MODULE_FORWARD(void, arm_set_nzcv_add, (uint32_t a, uint32_t b, uint32_t r), (a, b, r))
GBARECOMP_MODULE_FORWARD(void, arm_set_nzcv_adc, (uint32_t a, uint32_t b, uint32_t c, uint32_t r), (a, b, c, r))
GBARECOMP_MODULE_FORWARD(void, arm_set_nzcv_sub, (uint32_t a, uint32_t b, uint32_t r), (a, b, r))
GBARECOMP_MODULE_FORWARD(void, arm_set_nzcv_sbc, (uint32_t a, uint32_t b, uint32_t c, uint32_t r), (a, b, c, r))

// ── Bus and timing ──
GBARECOMP_MODULE_FORWARD(uint32_t, bus_read_u32, (uint32_t a), (a))
GBARECOMP_MODULE_FORWARD(uint16_t, bus_read_u16, (uint32_t a), (a))
GBARECOMP_MODULE_FORWARD(uint8_t, bus_read_u8, (uint32_t a), (a))
GBARECOMP_MODULE_FORWARD(void, bus_write_u32, (uint32_t a, uint32_t v), (a, v))
GBARECOMP_MODULE_FORWARD(void, bus_write_u16, (uint32_t a, uint16_t v), (a, v))
GBARECOMP_MODULE_FORWARD(void, bus_write_u8, (uint32_t a, uint8_t v), (a, v))
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_mem_cycles, (uint32_t a, uint32_t w, uint32_t s), (a, w, s))
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_mul_cycles, (uint32_t r, uint32_t s, uint32_t e), (r, s, e))
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_prefetch_stall_delta, (uint32_t w, uint32_t pc, uint32_t t), (w, pc, t))
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_exception_return_refill, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(void, runtime_tick, (uint32_t c), (c))
GBARECOMP_MODULE_FORWARD(bool, runtime_should_yield, (void), ())
GBARECOMP_MODULE_FORWARD(void, runtime_idle_backedge, (uint32_t pc), (pc))

// ── Dispatch and calls ──
GBARECOMP_MODULE_FORWARD(void, runtime_dispatch, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(void, runtime_dispatch_with_exchange, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(void, runtime_dispatch_miss, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(void, runtime_call_push_return, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(int, runtime_call_should_return, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(void, runtime_call_cancel_return, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(int, runtime_has_static_entry, (uint32_t pc, int t), (pc, t))
GBARECOMP_MODULE_FORWARD(int, runtime_bridge_interpret,
                         (uint32_t pc, bool t, uint32_t stop, uint64_t max), (pc, t, stop, max))
GBARECOMP_MODULE_FORWARD(void, runtime_force_interp_step, (void), ())
GBARECOMP_MODULE_FORWARD(int, gba_mod_function_entry, (uint32_t pc, int t, ArmCpuState* c), (pc, t, c))

// ── Exceptions and status registers ──
GBARECOMP_MODULE_FORWARD(void, runtime_swi, (uint32_t imm), (imm))
GBARECOMP_MODULE_FORWARD(void, runtime_irq, (uint32_t ret), (ret))
GBARECOMP_MODULE_FORWARD(void, runtime_exception_return, (uint32_t pc), (pc))
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_mrs_cpsr, (void), ())
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_mrs_spsr, (void), ())
GBARECOMP_MODULE_FORWARD(void, runtime_msr_cpsr, (uint32_t v, uint32_t m), (v, m))
GBARECOMP_MODULE_FORWARD(void, runtime_msr_spsr, (uint32_t v, uint32_t m), (v, m))
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_read_user_reg, (uint32_t r), (r))
GBARECOMP_MODULE_FORWARD(void, runtime_write_user_reg, (uint32_t r, uint32_t v), (r, v))

// ── Copied-RAM code ──
GBARECOMP_MODULE_FORWARD(int, runtime_native_ram_instruction_matches,
                         (uint32_t pc, uint32_t op, uint32_t sz), (pc, op, sz))
GBARECOMP_MODULE_FORWARD(int, runtime_native_ram_instruction_matches_masked,
                         (uint32_t pc, uint32_t op, uint32_t sz, uint32_t m), (pc, op, sz, m))
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_native_arm_immediate, (uint32_t pc), (pc))

// ── Tracing ──
GBARECOMP_MODULE_FORWARD(void, runtime_trace_event,
                         (uint32_t k, uint32_t pc, uint32_t a, uint32_t v, uint32_t x), (k, pc, a, v, x))
GBARECOMP_MODULE_FORWARD(void, runtime_insn_fp, (void), ())
GBARECOMP_MODULE_FORWARD(uint32_t, runtime_current_pc, (void), ())

// ── Rare paths ──
GBARECOMP_MODULE_FORWARD(void, runtime_unimplemented_op, (const char* op, uint32_t pc), (op, pc))
GBARECOMP_MODULE_FORWARD(void, runtime_note_interpreted_exception_return, (uint32_t m), (m))
GBARECOMP_MODULE_FORWARD(void, runtime_restore_cpsr_from_spsr, (void), ())

#undef GBARECOMP_MODULE_FORWARD

// ── Copied-RAM code views (same layout as src/armv4t/runtime_arm.h) ──
typedef const uint8_t* (*RuntimeRamCodeView)(uint32_t address, uint32_t size);
typedef struct RuntimeNativeRamCodeSource {
    RuntimeRamCodeView view;
    const uint8_t* ewram;
    const uint8_t* iwram;
} RuntimeNativeRamCodeSource;
#define g_runtime_native_ram_code_source     (*static_cast<RuntimeNativeRamCodeSource*>(gbarecomp_module_host.native_ram_code_source))
#define g_runtime_ram_code_view     (*static_cast<RuntimeRamCodeView*>(gbarecomp_module_host.ram_code_view))

// Fetch-wait helpers, shared verbatim with the host.
#include "runtime_wait_inline.h"

// ── Copied-RAM templates (same layout as src/armv4t/runtime_arm.h) ──
typedef struct RuntimeNativeRamTemplate {
    const uint8_t* bytes;
    uint32_t size;
    uint8_t thumb;
    void (*fn)(void);
    void (*cart_fn)(void);
    const uint8_t* masks;
} RuntimeNativeRamTemplate;
typedef struct RuntimeNativeRamEntry {
    uint64_t key;
    uint32_t template_index;
    uint32_t offset;
} RuntimeNativeRamEntry;

// Called by the generated tables' static initializers while the module loads;
// module_glue.cpp keeps what they pass and hands it over in gbarecomp_module_init.
void runtime_register_native_ram_templates(
    const RuntimeNativeRamTemplate* templates, uint32_t template_count,
    const RuntimeNativeRamEntry* entries, uint32_t entry_count);

}  // extern "C"

// Reads copied-RAM instructions straight from the live view when it is still
// the one this activation started with (a copy of src/armv4t/runtime_arm.h's).
class RuntimeNativeRamCodeView {
public:
    RuntimeNativeRamCodeView() : source_(g_runtime_native_ram_code_source) {}
    int matches(uint32_t pc, uint32_t opcode, uint32_t size,
                uint32_t mask = 0xFFFFFFFFu) const {
        const uint8_t* bytes = current_bytes(pc, size);
        if (!bytes)
            return runtime_native_ram_instruction_matches_masked(pc, opcode, size, mask);
        return ((load(bytes, size) ^ opcode) & mask) == 0;
    }
    uint32_t arm_immediate(uint32_t pc) const {
        const uint8_t* bytes = current_bytes(pc, 4u);
        if (!bytes) return runtime_native_arm_immediate(pc);
        const uint32_t encoded = uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8);
        const uint32_t value = encoded & 0xFFu, rotate = ((encoded >> 8) & 15u) * 2u;
        return rotate ? (value >> rotate) | (value << (32u - rotate)) : value;
    }
private:
    const uint8_t* current_bytes(uint32_t pc, uint32_t size) const {
        if ((size != 2u && size != 4u) || !source_.view ||
            g_runtime_ram_code_view != source_.view ||
            g_runtime_native_ram_code_source.view != source_.view) return nullptr;
        const uint32_t region = pc >> 24;
        const uint8_t* bytes;
        uint32_t mask;
        if (region == 2u) {
            bytes = source_.ewram; mask = 0x3FFFFu;
            if (bytes != g_runtime_native_ram_code_source.ewram) return nullptr;
        } else if (region == 3u) {
            bytes = source_.iwram; mask = 0x7FFFu;
            if (bytes != g_runtime_native_ram_code_source.iwram) return nullptr;
        } else return nullptr;
        const uint32_t offset = pc & mask;
        if (!bytes || offset > mask + 1u - size) return nullptr;
        return bytes + offset;
    }
    static uint32_t load(const uint8_t* bytes, uint32_t size) {
        uint32_t value = uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8);
        if (size == 4u) value |= (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
        return value;
    }
    RuntimeNativeRamCodeSource source_;
};

// Guaranteed tail transfers: the native expansion (see src/armv4t/runtime_arm.h).
#define GBARECOMP_TAIL_CALL(fn) fn(); return
#define GBARECOMP_TAIL_DISPATCH(pc) runtime_dispatch(pc); return
#define GBARECOMP_TAIL_DISPATCH_WITH_EXCHANGE(pc) runtime_dispatch_with_exchange(pc); return
#define GBARECOMP_TAIL_SWI(imm) runtime_swi(imm); return
