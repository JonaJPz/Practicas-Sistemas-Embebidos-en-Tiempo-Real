/**
 * @file    buttons.h
 * @brief   Interfaz para lectura de botones con debounce por software.
 *
 * Cada boton se configura con pull-up interno. El nivel activo es LOW
 * (el boton conecta el pin a GND al presionarse).
 *
 * Plataforma : ESP32 / ESP-IDF 5.x-6.x
 * Estandar   : C99
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include "driver/gpio.h"

/* ------------------------------------------------------------------ */
/*  Tipos publicos                                                     */
/* ------------------------------------------------------------------ */

/** Identificador de boton. */
typedef enum {
    BTN_ID_START_PAUSE = 0,   /**< Boton Start / Pause  */
    BTN_ID_DIRECTION   = 1,   /**< Boton Direccion      */
    BTN_ID_SPEED       = 2,   /**< Boton Velocidad      */
    BTN_ID_COUNT       = 3,   /**< Numero total de botones */
} btn_id_t;

/**
 * @brief Parametros para la tarea de un boton (pasados por pvParameters).
 */
typedef struct {
    btn_id_t    id;     /**< Identificador del boton */
    gpio_num_t  gpio;   /**< GPIO asignado           */
} btn_task_params_t;

/* ------------------------------------------------------------------ */
/*  API publica                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa los GPIOs de todos los botones.
 *
 * Configura pull-up interno y modo entrada.
 * Debe llamarse desde app_main() antes de crear tareas.
 */
void buttons_init(void);

/**
 * @brief Tarea FreeRTOS que monitorea un boton y ejecuta su accion.
 *
 * @param pvParameters  Puntero a btn_task_params_t con el boton asignado.
 *
 * @note Firma FIJA segun restricciones de la practica.
 */
void vTaskButton(void *pvParameters);

#endif /* BUTTONS_H */
