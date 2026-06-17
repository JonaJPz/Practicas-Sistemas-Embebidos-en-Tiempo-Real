/**
 * @file    app_config.h
 * @brief   Configuracion central de hardware y parametros del sistema.
 *
 * Todos los pines GPIO, periodos y constantes numericas del proyecto
 * se definen aqui para facilitar el mantenimiento y la portabilidad.
 *
 * 7 segmentos catodo comun — segmento activo en HIGH (1)
 *
 *
 * Plataforma : ESP32 / ESP-IDF 5.x-6.x / FreeRTOS
 * Estandar   : C99
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"

/* ------------------------------------------------------------------ */
/*  Pines del display 7 segmentos (catodo comun)                       */
/*  Cambia estos GPIOs segun tu cableado real                          */
/* ------------------------------------------------------------------ */

#define SEG_A   GPIO_NUM_2    /**< Segmento a */
#define SEG_B   GPIO_NUM_4    /**< Segmento b */
#define SEG_C   GPIO_NUM_5    /**< Segmento c */
#define SEG_D   GPIO_NUM_18   /**< Segmento d */
#define SEG_E   GPIO_NUM_19   /**< Segmento e */
#define SEG_F   GPIO_NUM_21   /**< Segmento f */
#define SEG_G   GPIO_NUM_22   /**< Segmento g */

/** Numero de segmentos del display. */
#define SEG_COUNT   7U

/* ------------------------------------------------------------------ */
/*  Pines de botones (pull-up interno, activo en LOW)                  */
/* ------------------------------------------------------------------ */

#define BTN_START_PAUSE  GPIO_NUM_25  /**< Boton Start / Pause       */
#define BTN_DIRECTION    GPIO_NUM_26  /**< Boton Direccion           */
#define BTN_SPEED        GPIO_NUM_27  /**< Boton Velocidad           */

/* ------------------------------------------------------------------ */
/*  Velocidades de conteo                                              */
/* ------------------------------------------------------------------ */

/** Periodo lento (ms) — velocidad 1 */
#define COUNTER_PERIOD_SLOW_MS   500U

/** Periodo rapido (ms) — velocidad 2 */
#define COUNTER_PERIOD_FAST_MS   250U

/* ------------------------------------------------------------------ */
/*  Debounce                                                           */
/* ------------------------------------------------------------------ */

/** Tiempo de debounce para botones en ms */
#define BTN_DEBOUNCE_MS   50U

/* ------------------------------------------------------------------ */
/*  Tamanos de stack de tareas (words)                                 */
/* ------------------------------------------------------------------ */

#define STACK_COUNTER_WORDS   2048U
#define STACK_BUTTON_WORDS    2048U
#define STACK_MANAGER_WORDS   2048U

/* ------------------------------------------------------------------ */
/*  Valor maximo del contador BCD                                      */
/* ------------------------------------------------------------------ */

#define COUNTER_MAX   9U
#define COUNTER_MIN   0U

#endif /* APP_CONFIG_H */
