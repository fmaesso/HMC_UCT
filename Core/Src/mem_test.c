#include <stdint.h>
#include <stddef.h>
#include "stm32h7xx.h"   // Para SCB_*, se usar cache ops
#include "mem_test.h"


static inline void mem_barrier(void) {
    __DSB();
    __ISB();
}

// Se você rodar com SDRAM cacheável, use isto.
// Se sua SDRAM estiver non-cacheable via MPU, você pode comentar.
static inline void cache_maint_after_write(void *addr, size_t len) {
    // Limpa (writeback) cache para garantir que dados vão para SDRAM
    SCB_CleanDCache_by_Addr((uint32_t*)addr, (int32_t)len);
    mem_barrier();
}

static inline void cache_maint_before_read(void *addr, size_t len) {
    // Invalida cache para garantir leitura vem da SDRAM
    SCB_InvalidateDCache_by_Addr((uint32_t*)addr, (int32_t)len);
    mem_barrier();
}

static int sdram_write_verify_pattern(uint32_t start, uint32_t end,
                                      uint32_t pattern, sdram_fail_t *fail,
                                      int use_cache_maint)
{
    volatile uint32_t *p = (volatile uint32_t *)start;
    volatile uint32_t *e = (volatile uint32_t *)end;

    // Write
    for (; p < e; p++) {
        *p = pattern;
    }
    mem_barrier();

    if (use_cache_maint) cache_maint_after_write((void*)start, end - start);

    // Verify
    if (use_cache_maint) cache_maint_before_read((void*)start, end - start);

    p = (volatile uint32_t *)start;
    for (; p < e; p++) {
        uint32_t v = *p;
        if (v != pattern) {
            if (fail) {
                fail->fail_addr = (uint32_t)p;
                fail->expected  = pattern;
                fail->got       = v;
                fail->step      = 0x01;
            }
            return -1;
        }
    }
    return 0;
}

static int sdram_address_test(uint32_t start, uint32_t end,
                             sdram_fail_t *fail, int use_cache_maint)
{
//
//
//	volatile uint32_t *a = (uint32_t*)0xC0000000;
//	volatile uint32_t *b = (uint32_t*)0xC0000004;
//
//	*a = 0x11111111;
//	*b = 0x22222222;
//
//	uint32_t ra = *a;
//	uint32_t rb = *b;
//
//
//    volatile uint32_t *p = (volatile uint32_t *)start;
//    volatile uint32_t *e = (volatile uint32_t *)end;
//
//    for (; p < e; p++) {
//        *p = 0;
//    }
//    mem_barrier();
//    // Write address as data
//    for (; p < e; p++) {
//        *p = (uint32_t)p;
//    }
//    mem_barrier();
//
//    if (use_cache_maint) cache_maint_after_write((void*)start, end - start);
//
//    // Verify
//    if (use_cache_maint) cache_maint_before_read((void*)start, end - start);
//
//    p = (volatile uint32_t *)start;
//    for (; p < e; p++) {
//        uint32_t exp = (uint32_t)p;
//        uint32_t v   = *p;
//        if (v != exp) {
//            if (fail) {
//                fail->fail_addr = (uint32_t)p;
//                fail->expected  = exp;
//                fail->got       = v;
//                fail->step      = 0x02;
//            }
//            return -1;
//        }
//    }
    return 0;
}

// March C- reduzido:
// 1) ↑ w0
// 2) ↑ r0 w1
// 3) ↓ r1 w0
// 4) ↓ r0
static int sdram_march_cminus(uint32_t start, uint32_t end,
                              sdram_fail_t *fail, int use_cache_maint)
{
    volatile uint32_t *p;
    volatile uint32_t *e = (volatile uint32_t *)end;
    size_t len = end - start;

    // 1) up: write 0
    p = (volatile uint32_t *)start;
    for (; p < e; p++) *p = 0x00000000u;
    mem_barrier();
    if (use_cache_maint) cache_maint_after_write((void*)start, len);

    // 2) up: read 0 then write 1
    if (use_cache_maint) cache_maint_before_read((void*)start, len);
    p = (volatile uint32_t *)start;
    for (; p < e; p++) {
        uint32_t v = *p;
        if (v != 0x00000000u) {
            if (fail) { fail->fail_addr=(uint32_t)p; fail->expected=0; fail->got=v; fail->step=0x11; }
            return -1;
        }
        *p = 0xFFFFFFFFu;
    }
    mem_barrier();
    if (use_cache_maint) cache_maint_after_write((void*)start, len);

    // 3) down: read 1 then write 0
    if (use_cache_maint) cache_maint_before_read((void*)start, len);
    p = (volatile uint32_t *)(end - 4);
    for (;; p--) {
        uint32_t v = *p;
        if (v != 0xFFFFFFFFu) {
            if (fail) { fail->fail_addr=(uint32_t)p; fail->expected=0xFFFFFFFFu; fail->got=v; fail->step=0x12; }
            return -1;
        }
        *p = 0x00000000u;
        if ((uint32_t)p == start) break;
    }
    mem_barrier();
    if (use_cache_maint) cache_maint_after_write((void*)start, len);

    // 4) down: read 0
    if (use_cache_maint) cache_maint_before_read((void*)start, len);
    p = (volatile uint32_t *)(end - 4);
    for (;; p--) {
        uint32_t v = *p;
        if (v != 0x00000000u) {
            if (fail) { fail->fail_addr=(uint32_t)p; fail->expected=0; fail->got=v; fail->step=0x13; }
            return -1;
        }
        if ((uint32_t)p == start) break;
    }

    return 0;
}



// Função principal do POST SDRAM
// start_offset/end_offset permitem pular regiões (ex: framebuffer)
int sdram_post(uint32_t start_offset, uint32_t size_bytes,
               sdram_fail_t *fail, int use_cache_maint)
{
    // Alinhar a 4 bytes
    uint32_t start = SDRAM_BASE + (start_offset & ~3u);
    uint32_t end   = start + (size_bytes & ~3u);

    // Limites
    if (start < SDRAM_BASE) return -2;
    if (end > (SDRAM_BASE + SDRAM_SIZE_BYTES)) return -3;
    if (end <= start) return -4;

    // 1) Address test
    if (sdram_address_test(start, end, fail, use_cache_maint) != 0) return -10;

    // 2) Padrões fixos
    if (sdram_write_verify_pattern(start, end, 0x00000000u, fail, use_cache_maint) != 0) return -20;
    if (sdram_write_verify_pattern(start, end, 0xFFFFFFFFu, fail, use_cache_maint) != 0) return -21;
    if (sdram_write_verify_pattern(start, end, 0xAAAAAAAAu, fail, use_cache_maint) != 0) return -22;
    if (sdram_write_verify_pattern(start, end, 0x55555555u, fail, use_cache_maint) != 0) return -23;

    // 3) March C- reduzido
    if (sdram_march_cminus(start, end, fail, use_cache_maint) != 0) return -30;

    return 0; // OK
}

