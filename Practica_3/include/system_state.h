/**
 * @file    system_state.h
 * @brief   Estado compartido del sistema entre tareas FreeRTOS.
 *
 * Encapsula el estado global del contador: valor actual, direccion,
 * velocidad y si el sistema esta activo o pausado.
 *
 * El acceso se protege con un mutex de FreeRTOS para evitar condiciones
 * de carrera entre la tarea contadora y las tareas de botones.
 *
 * Plataforma : ESP32 / FreeRTOS
 * Estandar   : C99
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/* ------------------------------------------------------------------ */
/*  Tipos publicos                                                     */
/* ------------------------------------------------------------------ */

/** Direccion de conteo. */
typedef enum {
    DIRECTION_UP   = 0,   /**< Conteo ascendente  0→9 */
    DIRECTION_DOWN = 1,   /**< Conteo descendente 9→0 */
} count_direction_t;

/** Velocidad de conteo. */
typedef enum {
    SPEED_SLOW = 0,   /**< 500 ms por digito */
    SPEED_FAST = 1,   /**< 250 ms por digito */
} count_speed_t;

/**
 * @brief Estado completo del sistema.
 *
 * Unica fuente de verdad compartida por todas las tareas.
 * Solo se accede a traves de system_state_get/set con mutex.
 */
typedef struct {
    uint8_t           value;       /**< Valor BCD actual [0, 9]      */
    count_direction_t direction;   /**< Direccion de conteo          */
    count_speed_t     speed;       /**< Velocidad de conteo          */
    bool              running;     /**< true = activo, false = pausa */
} system_state_t;

/* ------------------------------------------------------------------ */
/*  API publica                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa el modulo de estado del sistema.
 *
 * Crea el mutex interno. Debe llamarse desde app_main() antes de
 * crear cualquier tarea.
 */
void system_state_init(void);

/**
 * @brief Lee una copia del estado actual (thread-safe).
 *
 * @param out  Puntero donde se escribe la copia del estado.
 */
void system_state_get(system_state_t *out);

/**
 * @brief Escribe el estado completo (thread-safe).
 *
 * @param in  Puntero al nuevo estado a aplicar.
 */
void system_state_set(const system_state_t *in);

/**
 * @brief Incrementa o decrementa el valor BCD segun la direccion.
 *
 * Aplica wrap-around: 9→0 en ascendente, 0→9 en descendente.
 * Solo actua si el sistema esta en running. Thread-safe.
 */
void system_state_step(void);

#endif /* SYSTEM_STATE_H */
