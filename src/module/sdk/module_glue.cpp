// module_glue.cpp — compiled into every game module next to the generated
// sources (see ../gbarecomp_module_abi.h and docs/GAME_MODULES.md).
//
// Build with exactly one of
//   -DGBARECOMP_MODULE_CART   the cart code (kDispatchTable, copied-RAM templates)
//   -DGBARECOMP_MODULE_BIOS   the BIOS code (kBiosDispatchTable)
// and -DGBARECOMP_MODULE_SOURCE_SHA1="<sha1 of the ROM or BIOS>".
#include "runtime_arm.h"

#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
#define GBARECOMP_MODULE_EXPORT __declspec(dllexport)
#else
#define GBARECOMP_MODULE_EXPORT __attribute__((visibility("default")))
#endif

#if defined(GBARECOMP_MODULE_CART) == defined(GBARECOMP_MODULE_BIOS)
#error "define exactly one of GBARECOMP_MODULE_CART and GBARECOMP_MODULE_BIOS"
#endif
#ifndef GBARECOMP_MODULE_SOURCE_SHA1
#error "define GBARECOMP_MODULE_SOURCE_SHA1"
#endif

struct DispatchEntry;
struct GbaSymbol;

extern "C" {

GbaModuleHost gbarecomp_module_host;

#if defined(GBARECOMP_MODULE_CART)
extern const DispatchEntry kDispatchTable[];
extern const unsigned kDispatchTableLen;
#else
extern const DispatchEntry kBiosDispatchTable[];
extern const unsigned kBiosDispatchTableLen;
#endif

// What the generated tables register from their static initializers, which
// run while the module loads, before the host calls gbarecomp_module_init.
static const RuntimeNativeRamTemplate* s_ram_templates;
static uint32_t s_ram_template_count;
static const RuntimeNativeRamEntry* s_ram_entries;
static uint32_t s_ram_entry_count;
static const GbaSymbol* s_symbols;
static unsigned s_symbol_count;

void runtime_register_native_ram_templates(const RuntimeNativeRamTemplate* templates,
                                           uint32_t template_count,
                                           const RuntimeNativeRamEntry* entries,
                                           uint32_t entry_count) {
    s_ram_templates = templates;
    s_ram_template_count = template_count;
    s_ram_entries = entries;
    s_ram_entry_count = entry_count;
}

void gba_symbol_register_cart(const GbaSymbol* tab, unsigned count) {
    s_symbols = tab;
    s_symbol_count = count;
}

void gba_symbol_register_bios(const GbaSymbol* tab, unsigned count) {
    s_symbols = tab;
    s_symbol_count = count;
}

GBARECOMP_MODULE_EXPORT int gbarecomp_module_init(const GbaModuleHost* host,
                                                  GbaModuleTables* out) {
    if (!host || !out) return 0;
    memset(out, 0, sizeof(*out));
    if (host->abi_version != GBARECOMP_MODULE_ABI_VERSION || host->size != sizeof(GbaModuleHost))
        return 0;
    gbarecomp_module_host = *host;
    out->abi_version = GBARECOMP_MODULE_ABI_VERSION;
    out->size = sizeof(GbaModuleTables);
    out->source_sha1 = GBARECOMP_MODULE_SOURCE_SHA1;
#if defined(GBARECOMP_MODULE_CART)
    out->kind = kGbaModuleCart;
    out->dispatch = kDispatchTable;
    out->dispatch_count = kDispatchTableLen;
#else
    out->kind = kGbaModuleBios;
    out->dispatch = kBiosDispatchTable;
    out->dispatch_count = kBiosDispatchTableLen;
#endif
    out->ram_templates = s_ram_templates;
    out->ram_template_count = s_ram_template_count;
    out->ram_entries = s_ram_entries;
    out->ram_entry_count = s_ram_entry_count;
    out->symbols = s_symbols;
    out->symbol_count = s_symbol_count;
    return 1;
}

}  // extern "C"
