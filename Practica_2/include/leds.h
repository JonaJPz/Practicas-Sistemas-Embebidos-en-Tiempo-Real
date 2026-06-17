/**
 * @file    leds.h
 * @brief   Interfaz para el control de LEDs mediante PWM (LEDC) en ESP32.
 *
 * Proporciona funciones de inicializacion y escritura de ciclo de trabajo
 * para tres canales LED independientes. Cada canal utiliza un timer LEDC
 * de alta velocidad con resolucion de 8 bits (0-255).
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x
 * Estandar   : C99
 */

#ifndef LEDS_H
#define LEDS_H


#include "driver/gpio.h"    
#include "driver/ledc.h"

/* ------------------------------------------------------------------ */
/*  Mapeo de hardware                                                  */
/* ------------------------------------------------------------------ */

/** Pines GPIO asignados a cada LED.  */
#define LED_PIN_0   GPIO_NUM_25   /**< LED canal 0 */
#define LED_PIN_1   GPIO_NUM_26   /**< LED canal 1 */
#define LED_PIN_2   GPIO_NUM_27   /**< LED canal 2 */

/** Frecuencia del timer PWM en Hz. */
#define LED_PWM_FREQ_HZ     5000U

/** Resolucion del ciclo de trabajo: 8 bits -> rango 0-255. */
#define LED_PWM_RESOLUTION  LEDC_TIMER_8_BIT

/** Numero total de canales LED. */
#define LED_CHANNEL_COUNT   3U

/* ------------------------------------------------------------------ */
/*  Tipos publicos                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Identificador de canal LED.
 *
 * Corresponde al indice (0, 1 o 2) del arreglo interno de canales.
 */
typedef uint8_t led_channel_t;

/**
 * @brief  Inicializa el subsistema LEDC para los tres canales LED.
 *
 * Debe llamarse una sola vez desde main() antes de iniciar el scheduler.
 * Configura un timer compartido y un canal LEDC por cada LED.
 */
void leds_init(void);

/**
 * @brief  Establece el ciclo de trabajo (brillo) de un canal LED.
 *
 * @param channel  Indice del canal: 0, 1 o 2.
 * @param duty     Ciclo de trabajo de 8 bits (0 = apagado, 255 = maximo).
 *
 * @note   Si channel >= LED_CHANNEL_COUNT la funcion no hace nada.
 */
void leds_set_duty(led_channel_t channel, uint8_t duty);

#endif /* LEDS_H */