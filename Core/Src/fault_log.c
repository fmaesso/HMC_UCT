#include "main.h"
#include "fault_log.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx.h"

#include <stdio.h>
#include <string.h>

/* ========================= CONFIG ========================= */

#define FAULT_MAGIC    (0xFA17A5Bu)
#define FAULT_VERSION  (1u)
#define CACHE_LINE     (32u)
#define ALIGN_UP(x,a)  (((x) + ((a)-1u)) & ~((a)-1u))

/* ====================== REGISTRO ========================== */

typedef struct
{
  uint32_t magic;
  uint32_t version;
  uint32_t size_bytes;

  uint32_t uptime_ms;
  uint32_t active_icsr;

  fault_stack_t frame;

  uint32_t cfsr;
  uint32_t hfsr;
  uint32_t dfsr;
  uint32_t afsr;
  uint32_t bfar;
  uint32_t mmfar;

  uint32_t reserved[8];

  uint32_t crc_xor;  /* calculado excluindo este campo */
} fault_record_t;

/*
  Esta variável vai para 0x38800000 (BKPSRAM) via linker section.
  IMPORTANTE: você precisa ter adicionado a seção .BKP_SRAM_Section no flash.ld
*/
__attribute__((section(".BKP_SRAM_Section"), aligned(32)))
volatile fault_record_t g_fault_rec_bkp;

/* =================== BKPSRAM INIT ========================= */

static void BKPSRAM_Init(void)
{
  /* Clock do PWR (H7: APB4) */
#ifdef RCC_APB4ENR_PWREN
  RCC->APB4ENR |= RCC_APB4ENR_PWREN;
  (void)RCC->APB4ENR;
#endif

  /* Libera acesso ao domínio de backup */
#ifdef HAL_PWR_MODULE_ENABLED
  HAL_PWR_EnableBkUpAccess();
#else
#ifdef PWR_CR1_DBP
  PWR->CR1 |= PWR_CR1_DBP;
  for (volatile int i = 0; i < 1000; i++) { }
#endif
#endif

  /* Clock da BKPSRAM */
#ifdef RCC_AHB4ENR_BKPSRAMEN
  RCC->AHB4ENR |= RCC_AHB4ENR_BKPSRAMEN;
  (void)RCC->AHB4ENR;
#endif

  __DSB();
  __ISB();
}

/* ===================== XOR / CRC ========================== */

static uint32_t xor_words(const uint32_t *p, uint32_t words)
{
  uint32_t x = 0u;
  for (uint32_t i = 0; i < words; i++) x ^= p[i];
  return x;
}

static uint32_t faultrec_calc_xor_excluding_crc(const fault_record_t *r)
{
  const uint32_t *p = (const uint32_t *)r;
  uint32_t words_total = (uint32_t)(sizeof(fault_record_t) / 4u);
  return xor_words(p, words_total - 1u);
}

static int FaultRec_IsValid(const fault_record_t *r)
{
  if (r->magic != FAULT_MAGIC) return 0;
  if (r->version != FAULT_VERSION) return 0;
  if (r->size_bytes != sizeof(fault_record_t)) return 0;

  uint32_t calc = faultrec_calc_xor_excluding_crc(r);
  return (calc == r->crc_xor);
}

/* ======================= SAVE ============================= */

void FaultLog_SaveFromFrame(const fault_stack_t *frame,
                            uint32_t cfsr, uint32_t hfsr, uint32_t dfsr,
                            uint32_t afsr, uint32_t bfar, uint32_t mmfar,
                            uint32_t icsr,
                            uint32_t uptime_ms)
{
  BKPSRAM_Init();

  fault_record_t tmp;
  memset(&tmp, 0, sizeof(tmp));

  tmp.magic      = FAULT_MAGIC;
  tmp.version    = FAULT_VERSION;
  tmp.size_bytes = sizeof(tmp);

  tmp.uptime_ms   = uptime_ms;
  tmp.active_icsr = icsr;

  tmp.frame = *frame;

  tmp.cfsr  = cfsr;
  tmp.hfsr  = hfsr;
  tmp.dfsr  = dfsr;
  tmp.afsr  = afsr;
  tmp.bfar  = bfar;
  tmp.mmfar = mmfar;

  tmp.crc_xor = faultrec_calc_xor_excluding_crc(&tmp);

  /* Copia para BKPSRAM */
  volatile uint32_t *dst = (volatile uint32_t *)&g_fault_rec_bkp;
  const uint32_t *src    = (const uint32_t *)&tmp;

  for (uint32_t i = 0; i < sizeof(tmp)/4u; i++)
    dst[i] = src[i];

  /*
    Normalmente BKPSRAM não é cacheável, mas para ser “à prova de configuração”:
    (se sua MPU marcar como cacheável)
  */
  uint32_t sz = ALIGN_UP(sizeof(g_fault_rec_bkp), CACHE_LINE);
  SCB_CleanDCache_by_Addr((uint32_t *)&g_fault_rec_bkp, sz);

  __DSB();
  __ISB();
}

static const char* Fault_VectorName(uint32_t icsr)
{
  uint32_t vect = icsr & 0x1FFu; // VECTACTIVE
  switch (vect)
  {
    case 3:  return "HardFault";
    case 4:  return "MemManage";
    case 5:  return "BusFault";
    case 6:  return "UsageFault";
    default: return "Other/Thread";
  }
}

void Fault_PrintSummary(uint32_t icsr, uint32_t cfsr, uint32_t hfsr,
                        uint32_t bfar, uint32_t mmfar)
{
  printf("\r\n=== Fault Summario ===\r\n");
  printf("Exception: %s (VECTACTIVE=%lu)\r\n",
         Fault_VectorName(icsr), (unsigned long)(icsr & 0x1FFu));

  /* MemManage */
  uint32_t mmfsr = cfsr & 0xFFu;
  if (mmfsr)
  {
    printf("MemManage: ");
    if (mmfsr & (1u<<1)) printf("DACCVIOL ");
    if (mmfsr & (1u<<0)) printf("IACCVIOL ");
    if (mmfsr & (1u<<3)) printf("MUNSTKERR ");
    if (mmfsr & (1u<<4)) printf("MSTKERR ");
    if (mmfsr & (1u<<5)) printf("MLSPERR ");
    printf("\r\n");
    if (mmfsr & (1u<<7)) printf("  MMFAR=0x%08lX\r\n", (unsigned long)mmfar);
  }

  /* BusFault */
  uint32_t bfsr = (cfsr >> 8) & 0xFFu;
  if (bfsr)
  {
    printf("BusFault : ");
    if (bfsr & (1u<<1)) printf("PRECISERR ");
    if (bfsr & (1u<<2)) printf("IMPRECISERR ");
    if (bfsr & (1u<<0)) printf("IBUSERR ");
    if (bfsr & (1u<<3)) printf("UNSTKERR ");
    if (bfsr & (1u<<4)) printf("STKERR ");
    if (bfsr & (1u<<5)) printf("LSPERR ");
    printf("\r\n");
    if (bfsr & (1u<<7)) printf("  BFAR=0x%08lX\r\n", (unsigned long)bfar);
  }

  /* UsageFault */
  uint32_t ufsr = (cfsr >> 16) & 0xFFFFu;
  if (ufsr)
  {
    printf("UsageFault: ");
    if (ufsr & (1u<<9)) printf("DIVBYZERO ");
    if (ufsr & (1u<<8)) printf("UNALIGNED ");
    if (ufsr & (1u<<0)) printf("UNDEFINSTR ");
    if (ufsr & (1u<<1)) printf("INVSTATE ");
    if (ufsr & (1u<<2)) printf("INVPC ");
    if (ufsr & (1u<<3)) printf("NOCP ");
    printf("\r\n");
  }

  if (hfsr & (1u<<30)) printf("HardFault: FORCED\r\n");
  if (hfsr & (1u<<31)) printf("HardFault: DEBUGEVT\r\n");
  if (hfsr & (1u<<1))  printf("HardFault: VECTTBL\r\n");

  printf("=====================\r\n");
}


/* ======================= PRINT ============================ */

void FaultLog_Print(void)
{
  BKPSRAM_Init();

  uint32_t sz = ALIGN_UP(sizeof(g_fault_rec_bkp), CACHE_LINE);
  SCB_InvalidateDCache_by_Addr((uint32_t *)&g_fault_rec_bkp, sz);
  __DSB(); __ISB();

  const fault_record_t *r = (const fault_record_t *)&g_fault_rec_bkp;

  uint32_t calc_now = faultrec_calc_xor_excluding_crc(r);

  if (!FaultRec_IsValid(r))
  {
    printf("BKPSRAM FaultLog: registro INVALIDO.\r\n");
    printf(" magic=0x%08lX ver=%lu size=%lu\r\n",
           (unsigned long)r->magic,
           (unsigned long)r->version,
           (unsigned long)r->size_bytes);
    printf(" crc_salvo=0x%08lX crc_calc=0x%08lX\r\n",
           (unsigned long)r->crc_xor,
           (unsigned long)calc_now);
    return;
  }

  printf("\r\n===== FAULT LOG (BKPSRAM 0x38800000) =====\r\n");
  printf("Uptime(ms): %lu\r\n", (unsigned long)r->uptime_ms);
  printf("ICSR      : 0x%08lX\r\n", (unsigned long)r->active_icsr);

  printf("PC   : 0x%08lX\r\n", (unsigned long)r->frame.pc);
  printf("LR   : 0x%08lX\r\n", (unsigned long)r->frame.lr);
  printf("PSR  : 0x%08lX\r\n", (unsigned long)r->frame.psr);

  printf("R0   : 0x%08lX\r\n", (unsigned long)r->frame.r0);
  printf("R1   : 0x%08lX\r\n", (unsigned long)r->frame.r1);
  printf("R2   : 0x%08lX\r\n", (unsigned long)r->frame.r2);
  printf("R3   : 0x%08lX\r\n", (unsigned long)r->frame.r3);
  printf("R12  : 0x%08lX\r\n", (unsigned long)r->frame.r12);

  printf("CFSR : 0x%08lX\r\n", (unsigned long)r->cfsr);
  printf("HFSR : 0x%08lX\r\n", (unsigned long)r->hfsr);
  printf("DFSR : 0x%08lX\r\n", (unsigned long)r->dfsr);
  printf("AFSR : 0x%08lX\r\n", (unsigned long)r->afsr);
  printf("BFAR : 0x%08lX\r\n", (unsigned long)r->bfar);
  printf("MMFAR: 0x%08lX\r\n", (unsigned long)r->mmfar);

  Fault_PrintDecoded(r->cfsr, r->hfsr, r->bfar, r->mmfar);

  Fault_PrintSummary(r->active_icsr, r->cfsr, r->hfsr, r->bfar, r->mmfar);

//  printf("CRC salvo: 0x%08lX | CRC calc: 0x%08lX\r\n",
//         (unsigned long)r->crc_xor,
//         (unsigned long)calc_now);

  printf("=========================================\r\n");
}

/* ======================= CLEAR ============================ */

void FaultLog_Clear(void)
{
  BKPSRAM_Init();

  volatile uint32_t *p = (volatile uint32_t *)&g_fault_rec_bkp;
  for (uint32_t i = 0; i < sizeof(fault_record_t)/4u; i++)
    p[i] = 0u;

  uint32_t sz = ALIGN_UP(sizeof(g_fault_rec_bkp), CACHE_LINE);
  SCB_CleanDCache_by_Addr((uint32_t *)&g_fault_rec_bkp, sz);

  __DSB();
  __ISB();
}

/* =================== DECODE HUMANO ========================= */

static void print_mmfsr(uint32_t cfsr, uint32_t mmfar)
{
  uint32_t mmfsr = (cfsr & 0xFFu);
  if (!mmfsr) return;

  printf(" [MemManage]\r\n");
  if (mmfsr & (1u<<0)) printf("  - IACCVIOL   Execucao em regiao proibida\r\n");
  if (mmfsr & (1u<<1)) printf("  - DACCVIOL   Acesso a dado invalido\r\n");
  if (mmfsr & (1u<<3)) printf("  - MUNSTKERR  Erro no unstack\r\n");
  if (mmfsr & (1u<<4)) printf("  - MSTKERR    Erro no stack\r\n");
  if (mmfsr & (1u<<5)) printf("  - MLSPERR    Lazy FPU error\r\n");
  if (mmfsr & (1u<<7)) printf("  - MMFAR = 0x%08lX\r\n", (unsigned long)mmfar);
}

static void print_bfsr(uint32_t cfsr, uint32_t bfar)
{
  uint32_t bfsr = (cfsr >> 8) & 0xFFu;
  if (!bfsr) return;

  printf(" [BusFault]\r\n");
  if (bfsr & (1u<<0)) printf("  - IBUSERR     Erro em fetch de instrucao\r\n");
  if (bfsr & (1u<<1)) printf("  - PRECISERR   Erro preciso (BFAR costuma ser valido)\r\n");
  if (bfsr & (1u<<2)) printf("  - IMPRECISERR Erro impreciso\r\n");
  if (bfsr & (1u<<3)) printf("  - UNSTKERR    Unstack error\r\n");
  if (bfsr & (1u<<4)) printf("  - STKERR      Stack error\r\n");
  if (bfsr & (1u<<5)) printf("  - LSPERR      Lazy FPU error\r\n");
  if (bfsr & (1u<<7)) printf("  - BFAR = 0x%08lX\r\n", (unsigned long)bfar);
}

static void print_ufsr(uint32_t cfsr)
{
  uint32_t ufsr = (cfsr >> 16) & 0xFFFFu;
  if (!ufsr) return;

  printf(" [UsageFault]\r\n");
  if (ufsr & (1u<<0)) printf("  - UNDEFINSTR\r\n");
  if (ufsr & (1u<<1)) printf("  - INVSTATE\r\n");
  if (ufsr & (1u<<2)) printf("  - INVPC\r\n");
  if (ufsr & (1u<<3)) printf("  - NOCP\r\n");
  if (ufsr & (1u<<8)) printf("  - UNALIGNED\r\n");
  if (ufsr & (1u<<9)) printf("  - DIVBYZERO\r\n");
}

void Fault_PrintDecoded(uint32_t cfsr, uint32_t hfsr, uint32_t bfar, uint32_t mmfar)
{
  printf("\r\n--- Fault Decode ---\r\n");

  print_mmfsr(cfsr, mmfar);
  print_bfsr(cfsr, bfar);
  print_ufsr(cfsr);

  if (hfsr & (1u<<30)) printf(" [HardFault] FORCED (veio de MM/Bus/Usage)\r\n");
  if (hfsr & (1u<<1))  printf(" [HardFault] VECTTBL (falha na tabela de vetores)\r\n");
  if (hfsr & (1u<<31)) printf(" [HardFault] DEBUGEVT (evento de debug)\r\n");

  printf("--------------------\r\n");
}
