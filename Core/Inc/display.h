///*
// * display.h
// *
// *  Created on: 6 de fev. de 2026
// *      Author: ferna
// */
//
//#ifndef INC_DISPLAY_H_
//#define INC_DISPLAY_H_
//
//
//typedef struct {
//    uint16_t hsync, hbp, hfp;
//    uint16_t vsync, vbp, vfp;
//
//    uint8_t  hs_pol_low;
//    uint8_t  vs_pol_low;
//    uint8_t  de_pol_high;
//    uint8_t  pclk_falling;
//    uint8_t  pol_set;
//} ltdc_timing_t;
//
//
//typedef struct {
//    uint32_t M;
//    uint32_t N;
//    uint32_t R;
//    uint32_t freq_hz;     // frequência real obtida
//    int32_t  error_hz;    // erro em Hz (obtida - desejada)
//} ltdc_clk_result_t;
//
//int LTDC_ParseCommand(char *cmd, ltdc_timing_t *t);
//void LTDC_SetFreq(char *S);
//void LTDC_Tune_ProcessLine(const char *line);
//int LTDC_SetPixelClock_Hz(uint32_t target_hz, ltdc_clk_result_t *res);
//
//#endif /* INC_DISPLAY_H_ */
