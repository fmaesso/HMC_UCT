/*
 * mem_test.h
 *
 *  Created on: Dec 22, 2025
 *      Author: ferna
 */

#ifndef INC_MEM_TEST_H_
#define INC_MEM_TEST_H_




// ===== Ajuste para seu hardware =====
#define SDRAM_BASE        (0xC0000000UL)
#define SDRAM_SIZE_BYTES  (0x01000000UL)  // 16 MB

// Se você quiser pular uma região (ex: framebuffer), use offsets.
typedef struct {
    uint32_t fail_addr;
    uint32_t expected;
    uint32_t got;
    uint32_t step;
} sdram_fail_t;

int sdram_post(uint32_t start_offset, uint32_t size_bytes,
               sdram_fail_t *fail, int use_cache_maint);


#endif /* INC_MEM_TEST_H_ */
