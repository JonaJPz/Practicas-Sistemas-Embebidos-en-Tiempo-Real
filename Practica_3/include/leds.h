/**
 * @file    leds.h
 * @brief   Interfaz del display de 7 segmentos (catodo comun).
 *
 * Proporciona funciones para inicializar los GPIOs y mostrar
 * un digito BCD (0-9) en el display.
 *
 * Catodo comun: segmento encendido = GPIO en HIGH (1).
 *
 * Plataforma : ESP32 / ESP-IDF 5.x-6.x
 * Estandar   : C99
 */

#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>

/**
 * @brief Inicializa los 7 GPIOs del display como salidas.
 *
 * Debe llamarse una sola vez desde app_main() antes de crear tareas.
 */
void leds_init(void);

/**
 * @brief Muestra un digito BCD en el display de 7 segmentos.
 *
 * @param digit  Valor a mostrar, rango [0, 9].
 *               Valores fuera de rango apagan el display.
 */
void leds_show_digit(uint8_t digit);

/**
 * @brief Apaga todos los segmentos del display.
 */
void leds_clear(void);

#endif /* LEDS_H */
