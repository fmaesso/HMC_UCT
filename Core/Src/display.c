///*
// * display.c
// *
// *  Created on: 6 de fev. de 2026
// *      Author: ferna
// */
//
//#include "main.h"
//#include <string.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include "display.h"
//#include "utils.h"
//
//extern LTDC_HandleTypeDef hltdc;
//extern char StrValores[MAX_STRS][MAX_STRS_LEN];
//
//#define LCD_W 1024
//#define LCD_H 600
//
//static void LTDC_ApplyTiming(const ltdc_timing_t *t)
//{
//    uint32_t HS  = t->hsync;
//    uint32_t HBP = t->hbp;
//    uint32_t HFP = t->hfp;
//
//    uint32_t VS  = t->vsync;
//    uint32_t VBP = t->vbp;
//    uint32_t VFP = t->vfp;
//
//    uint32_t AccHBP     = HS + HBP;
//    uint32_t AccActiveW = AccHBP + LCD_W;
//    uint32_t TotalW     = AccActiveW + HFP;
//
//    uint32_t AccVBP     = VS + VBP;
//    uint32_t AccActiveH = AccVBP + LCD_H;
//    uint32_t TotalH     = AccActiveH + VFP;
//
//    __HAL_LTDC_DISABLE(&hltdc);
//
//    // Sync size
//    hltdc.Instance->SSCR =
//        ((HS - 1U) << LTDC_SSCR_HSW_Pos) |
//        ((VS - 1U) << LTDC_SSCR_VSH_Pos);
//
//    // Back porch
//    hltdc.Instance->BPCR =
//        ((AccHBP - 1U) << LTDC_BPCR_AHBP_Pos) |
//        ((AccVBP - 1U) << LTDC_BPCR_AVBP_Pos);
//
//    // Active area
//    hltdc.Instance->AWCR =
//        ((AccActiveW - 1U) << LTDC_AWCR_AAW_Pos) |
//        ((AccActiveH - 1U) << LTDC_AWCR_AAH_Pos);
//
//    // Total area
//    hltdc.Instance->TWCR =
//        ((TotalW - 1U) << LTDC_TWCR_TOTALW_Pos) |
//        ((TotalH - 1U) << LTDC_TWCR_TOTALH_Pos);
//
//    // Polarities (opcional)
//    if (t->pol_set) {
//        uint32_t gcr = hltdc.Instance->GCR;
//
//        // HSYNC polarity
//        if (t->hs_pol_low) gcr &= ~LTDC_GCR_HSPOL;
//        else               gcr |=  LTDC_GCR_HSPOL;
//
//        // VSYNC polarity
//        if (t->vs_pol_low) gcr &= ~LTDC_GCR_VSPOL;
//        else               gcr |=  LTDC_GCR_VSPOL;
//
//        // DE polarity
//        if (t->de_pol_high) gcr |=  LTDC_GCR_DEPOL;
//        else                gcr &= ~LTDC_GCR_DEPOL;
//
//        // Pixel clock polarity
//        if (t->pclk_falling) gcr |=  LTDC_GCR_PCPOL;
//        else                 gcr &= ~LTDC_GCR_PCPOL;
//
//        hltdc.Instance->GCR = gcr;
//    }
//
//    // Reload imediato dos shadow regs
//    hltdc.Instance->SRCR = LTDC_SRCR_IMR;
//
//    __HAL_LTDC_ENABLE(&hltdc);
//}
//
//static int parse_u16(const char *s, uint16_t *out)
//{
//    char *end = NULL;
//    long v = strtol(s, &end, 10);
//    if (end == s || v < 0 || v > 2000) return -1;
//    *out = (uint16_t)v;
//    return 0;
//}
//
//int LTDC_ParseCommand(char *cmd, ltdc_timing_t *t)
//{
//	//L;H=4;B=50;F=100;V=4;b=23;f=12;P=0;VS=0;DE=1;K=1
//	//L;4;50;100;4;23;12;0;0;1;1
//
//	t->pol_set = 0;
//
//    GetDados(cmd, 2);
//    t->hsync = atoi(StrValores[0]);
//    t->hbp = atoi(StrValores[1]);
//    t->hfp = atoi(StrValores[2]);
//    t->vsync = atoi(StrValores[3]);
//    t->vbp = atoi(StrValores[4]);
//    t->vfp = atoi(StrValores[5]);
//
//    t->pol_set = 1;
//	t->hs_pol_low = atoi(StrValores[6]);
//	t->vs_pol_low = atoi(StrValores[7]);
//	t->de_pol_high = atoi(StrValores[8]);
//	t->pclk_falling = atoi(StrValores[9]);
//    return 0;
//}
//
//
//void LTDC_Tune_ProcessLine(const char *line)
//{
//    ltdc_timing_t t;
//    int rc = LTDC_ParseCommand(line, &t);
//    if (rc == 0) {
//        LTDC_ApplyTiming(&t);
//        printf("OK\r\n");
//    } else {
//        printf("ERRO\r\n");
//    }
//}
//
//static void PLL3_ReadCurrent(uint32_t *M, uint32_t *N)
//{
//    uint32_t divm3 = (RCC->PLLCKSELR & RCC_PLLCKSELR_DIVM3_Msk)
//                   >> RCC_PLLCKSELR_DIVM3_Pos;
//    if (divm3 == 0) divm3 = 1;
//    *M = divm3;
//
//    uint32_t pll3divr = RCC->PLL3DIVR;
//    *N = ((pll3divr & RCC_PLL3DIVR_N3_Msk)
//        >> RCC_PLL3DIVR_N3_Pos) + 1U;
//}
//
/////* Ajusta pixel clock do LTDC e retorna parâmetros usados */
////int LTDC_SetPixelClock_Hz(uint32_t target_hz, ltdc_clk_result_t *res)
////{
////    if (!res) return -1;
////
////    uint32_t M, N;
////    PLL3_ReadCurrent(&M, &N);
////
////    uint32_t bestR = 0;
////    uint64_t bestErr = (uint64_t)-1;
////    uint64_t bestFreq = 0;
////
////    for (uint32_t R = 2; R <= 128; R++) {
////        uint64_t fout =
////            ((uint64_t)HSE_VALUE * (uint64_t)N) /
////            ((uint64_t)M * (uint64_t)R);
////
////        uint64_t err = (fout > target_hz)
////                     ? (fout - target_hz)
////                     : (target_hz - fout);
////
////        if (err < bestErr) {
////            bestErr  = err;
////            bestR    = R;
////            bestFreq = fout;
////        }
////    }
////
////    if (bestR == 0) return -2;
////
////    /* Preenche retorno */
////    res->M = M;
////    res->N = N;
////    res->R = bestR;
////    res->freq_hz  = (uint32_t)bestFreq;
////    res->error_hz = (int32_t)((int64_t)bestFreq - (int64_t)target_hz);
////
////    /* Aplica no hardware */
////    __HAL_LTDC_DISABLE(&hltdc);
////
////    RCC_PeriphCLKInitTypeDef per = {0};
////    per.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
////    per.LtdcClockSelection   = RCC_;
////
////    per.PLL3.PLL3M = M;
////    per.PLL3.PLL3N = N;
////    per.PLL3.PLL3P = 2;
////    per.PLL3.PLL3Q = 2;
////    per.PLL3.PLL3R = bestR;
////
////    per.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
////
////    uint32_t fin = HSE_VALUE / M;
////    if      (fin <= 2000000UL)  per.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
////    else if (fin <= 4000000UL)  per.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_1;
////    else if (fin <= 8000000UL)  per.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;
////    else if (fin <= 16000000UL) per.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
////    else                        per.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_4;
////
////    if (HAL_RCCEx_PeriphCLKConfig(&per) != HAL_OK) {
////        __HAL_LTDC_ENABLE(&hltdc);
////        return -3;
////    }
////
////    __HAL_LTDC_ENABLE(&hltdc);
////    return 0;
////}
////
////void LTDC_SetFreq(char *S){
////	ltdc_clk_result_t r;
////	uint32_t f = atoi(S) * 1000;
////
////	if (LTDC_SetPixelClock_Hz(f, &r) == 0) {
////		printf("PLL3: M=%lu N=%lu R=%lu\r\n", r.M, r.N, r.R);
////		printf("PCLK real = %lu Hz (erro %+ld Hz)\r\n",
////			   r.freq_hz, r.error_hz);
////	}
////}
//
