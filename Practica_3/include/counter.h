/**
 * @file    counter.h
 * @brief   Interfaz de la tarea FreeRTOS del contador BCD.
 *
 * La tarea lee el estado del sistema, avanza el contador y
 * actualiza el display 7 segmentos. El periodo de ejecucion
 * lo determina la velocidad configurada en system_state.
 *
 * Plataforma : ESP32 / FreeRTOS
 * Estandar   : C99
 */

#ifndef COUNTER_H
#define COUNTER_H

/**
 * @brief Tarea FreeRTOS del contador BCD.
 *
 * Lee el system_state, avanza el valor segun la direccion y
 * periodo configurados, y actualiza el display.
 *
 * @param pvParameters  No utilizado (NULL). Firma fija de FreeRTOS.
 *
 * @note Firma FIJA segun restricciones de la practica.
 */
void vTaskCounter(void *pvParameters);

#endif /* COUNTER_H */
