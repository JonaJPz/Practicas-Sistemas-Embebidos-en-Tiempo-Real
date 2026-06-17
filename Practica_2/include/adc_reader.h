/**
 * @file    adc_reader.h
 * @brief   Interfaz para lectura de potenciometros via ADC en ESP32.
 *
 * Abstrae la configuracion del ADC de una sola vez y expone una funcion
 * de lectura por canal. Devuelve el valor en bruto (0-4095, 12 bits)
 * que las tareas escalan al rango de duty PWM (0-255).
 *
 * Canales utilizados:
 *   Canal 0 -> ADC1_CHANNEL_0  (GPIO 36 / SENSOR_VP)
 *   Canal 1 -> ADC1_CHANNEL_3  (GPIO 39 / SENSOR_VN)
 *   Canal 2 -> ADC1_CHANNEL_6  (GPIO 34)
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x
 * Estandar   : C99
 */

#ifndef ADC_READER_H
#define ADC_READER_H

#include <stdint.h>
#include "esp_adc/adc_oneshot.h"

/* ------------------------------------------------------------------ */
/*  Configuracion de hardware                                          */
/* ------------------------------------------------------------------ */

/** Numero de canales ADC utilizados. */
#define ADC_CHANNEL_COUNT   3U

/** Resolucion del ADC en bits (12 bits -> 0-4095). */
#define ADC_RESOLUTION_BITS 12U

/** Valor maximo del ADC segun la resolucion configurada. */
#define ADC_MAX_VALUE       ((1U << ADC_RESOLUTION_BITS) - 1U)   /* 4095 */

/** Canales ADC1 asignados a cada potenciometro (indices 0, 1, 2). */
#define ADC_CH_POT0   ADC_CHANNEL_0   /**< GPIO 36 */
#define ADC_CH_POT1   ADC_CHANNEL_3   /**< GPIO 39 */
#define ADC_CH_POT2   ADC_CHANNEL_6   /**< GPIO 34 */

/* ------------------------------------------------------------------ */
/*  API publica                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief  Inicializa el ADC1 en modo oneshot para los tres canales.
 *
 * Debe llamarse una sola vez desde main() antes de iniciar el scheduler.
 * Configura atenuacion de 11 dB (rango efectivo ~0-3.3 V) en cada canal.
 */
void adc_reader_init(void);

/**
 * @brief  Lee el valor crudo del potenciometro indicado.
 *
 * @param  channel  Indice del canal: 0, 1 o 2.
 * @return Valor ADC de 12 bits en el rango [0, 4095].
 *         Devuelve 0 si el canal esta fuera de rango.
 */
uint16_t adc_reader_get_raw(uint8_t channel);

/**
 * @brief  Escala un valor ADC de 12 bits al rango de duty PWM de 8 bits.
 *
 * @param  raw  Valor ADC en [0, 4095].
 * @return Valor duty en [0, 255].
 *
 * @note   Funcion inline utilitaria; no requiere estado interno.
 */
static inline uint8_t adc_reader_to_duty(uint16_t raw)
{
    /* Escalar de 12 bits a 8 bits: duty = raw * 255 / 4095 */
    return (uint8_t)((raw * 255U) / ADC_MAX_VALUE);
}

#endif /* ADC_READER_H */