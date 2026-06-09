/*
 * press.c
 *
 *  Created on: 10 de mar. de 2026
 *      Author: ferna
 */

#include "main.h"
#include "press.h"
#include "cec.h"
#include <stdlib.h>
extern ADC_HandleTypeDef hadc1;

//adc_type adc_buffer[2]; // Buffer preenchido pelo DMA (2 canais)
//TPressao SensoresPressao[2];

__attribute__((section(".sram4_section"))) TPressao SensoresPressao[2];
__attribute__((section(".sram4_section"))) adc_type adc_buffer[2];


//#define ADC_MAX_VAL      65535.0f  // 16-bit ADC do H7
#define ADC_MAX_VAL      1048575.0f
#define VREF             3.0f
#define GAIN_AMPOP       0.3905f   // R39/R37
#define ATT_DIVIDER      0.3329f   // R38/(R36+R38)
#define PSI_TO_MMHG      51.7149f
#define QUANT_MEDIA		30

void Sensor_UpdateAverage(TPressao *p, adc_type novo_valor);


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
    	if(SensoresPressao[0].Status){
    		Sensor_UpdateAverage(&SensoresPressao[0], adc_buffer[0]);
    	}else{
    		Sensor_CalibrateZero(&SensoresPressao[0], adc_buffer[0]);
    	}

        // 3. Atualiza o Sensor 2 (Canal 1)
    	if(SensoresPressao[1].Status){
            Sensor_UpdateAverage(&SensoresPressao[1], adc_buffer[1]);
    	}else{
    		Sensor_CalibrateZero(&SensoresPressao[1], adc_buffer[1]);
    	}
        HAL_ADC_Stop_DMA(&hadc1);
    }
}

void Sensor_UpdateAverage(TPressao *p, adc_type novo_valor)
{
    // 1. Converter leitura bruta para Tensão Real (Considerando o seu Oversampling)
    // Max ADC = 1.048.575 (16-bit + 64x ratio / 4 shift)
    float v_adc_inst = ((float)novo_valor) * (VREF / ADC_MAX_VAL);

    // 2. Converter para PSI (Sem R38, Ganho AmpOp 0.3905)
    float psi_inst = ((v_adc_inst - p->V_Zero_ADC) / 0.3905f) * 7.5f;
    float mmhg_inst = psi_inst * 51.7149f;

    // 3. FILTRO EMA (A "Mágica")
    // Formula: Valor = (Novo * Alfa) + (Antigo * (1 - Alfa))
    // Alfa próximo de 1.0 = Reage instantaneamente (com ruído)
    // Alfa próximo de 0.01 = Muito estável (lento)

    float alfa = 0.08f; // Comece com 0.08 (filtro forte mas responsivo)

    p->PressaoFiltrada = (mmhg_inst * alfa) + (p->PressaoFiltrada * (1.0f - alfa));

    // 4. Atribuir ao valor de exibição
    p->Pressao = p->PressaoFiltrada;
}

void Sensor_CalibrateZero(TPressao *p, adc_type raw_adc_ptr)
{
	static uint64_t acumulador[2] = {0u, 0u};
    static uint16_t num_amostras[2] = {0u, 0u};
    uint32_t idx = 0u;

    if (p == &SensoresPressao[1]) {
    	idx = 1u;
    }

	num_amostras[idx]++;
	acumulador[idx] += raw_adc_ptr;
	if(num_amostras[idx] <= 100){
		return;
	}
    // 2. Calcula a média das leituras brutas (20 bits do oversampling)
    float media_adc_zero = (float)acumulador[idx] / (float)num_amostras[idx];

    // 3. Converte essa média para Volts e armazena como o Offset de Zero
    // Max ADC = 1048575 (16-bit + 64x ratio / 4 shift)
    p->V_Zero_ADC = (media_adc_zero * VREF) / 1048575.0f;

    // 4. Inicializa o filtro EMA com o valor de zero para evitar saltos na tela
    p->PressaoFiltrada = 0.0f;
    p->Status = 1; // Calibrado
}

//void Sensor_UpdateAverage(TPressao *p, uint32_t novo_valor) // Mudei para uint32_t pois o valor agora > 65535
//{
//    // 1. Média Móvel Otimizada
//    p->Soma -= p->valMedia[p->Index];
//    p->valMedia[p->Index] = (uint16_t)(novo_valor >> 4); // Se quiseres manter o buffer em 16bits, mas perdes precisão do oversampling
//    // O IDEAL: Altera p->valMedia para uint32_t na struct
//
//    // Assumindo que alteraste valMedia para uint32_t na struct:
//    p->valMedia[p->Index] = novo_valor;
//    p->Soma += novo_valor;
//
//    p->Index++;
//    if (p->Index >= QUANT_MEDIA) p->Index = 0;
//
//    // 2. Conversão para Tensão (Usando o novo máximo)
//    float v_adc_media = ((float)p->Soma / QUANT_MEDIA) * (VREF / ADC_MAX_VAL);
//
//    // 3. Conversão para mmHg (Mantendo a lógica sem R38)
//    const float fator_ganho_hw = 0.3905f;
//    float delta_v_sensor = (v_adc_media - p->V_Zero_ADC) / fator_ganho_hw;
//    float psi = delta_v_sensor * (30.0f / 4.0f);
//
//    p->Pressao = psi * 51.7149f;
//}

//uint32_t LIMIAR_RUIDO = 300; // Ajuste esse valor conforme a agressividade desejada (em counts do ADC)
//#define MAX_REJEICOES	5

//void Sensor_UpdateAverage(TPressao *p, adc_type novo_valor)
//{
//	static uint8_t contadorRejeicao = 0;
//    // --- FILTRO DE DISCREPÂNCIA (OUTLIER REJECTION) ---
//    // Se não for a primeira leitura, verifica a variação
//    if (p->ultimoValorValido != 0) {
//        int16_t variacao = (int16_t)(novo_valor - p->ultimoValorValido);
//
//        // Se a variação for maior que o limiar, ignoramos o novo_valor
//        // e usamos o último válido para não "congelar" o filtro.
//        if (abs(variacao) > LIMIAR_RUIDO) {
//            novo_valor = p->ultimoValorValido;
//            contadorRejeicao++;
//            if (contadorRejeicao < MAX_REJEICOES) {
//				novo_valor = p->ultimoValorValido; // Descarta
//			} else {
//				contadorRejeicao = 0; // Aceita o novo patamar
//			}
//        }
//    }
//    p->ultimoValorValido = novo_valor;
//    // --------------------------------------------------
//
//    // 1. Média Móvel Otimizada O(1)
//    p->Soma -= p->valMedia[p->Index];
//    p->valMedia[p->Index] = novo_valor;
//    p->Soma += novo_valor;
//
//    p->Index++;
//    if (p->Index >= QUANT_MEDIA) p->Index = 0;
//
//    // 2. Conversão (Usando a sua fórmula sem o R38)
//    float v_adc_media = ((float)p->Soma / QUANT_MEDIA) * (VREF / ADC_MAX_VAL);
//    const float fator_ganho_hw = 0.3905f;
//
//    float delta_v_sensor = (v_adc_media - p->V_Zero_ADC) / fator_ganho_hw;
//    float psi = delta_v_sensor * (30.0f / 4.0f);
//
//    p->Pressao = psi * 51.7149f;
//}

// Chame isso uma vez no startup com o sistema em repouso
//void Sensor_CalibrateZero(TPressao *p) {
//    float v_zero_atual = ((float)p->Soma / QUANT_MEDIA) * (VREF / ADC_MAX_VAL);
//    p->V_Zero_ADC = v_zero_atual;
//}
