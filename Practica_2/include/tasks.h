/**
 * @file    tasks.h
 * @brief   Interfaz de tareas FreeRTOS para el control de LEDs por ADC.
 *
 * Cada tarea recibe un puntero void* (pvParameters) que apunta a una
 * estructura task_params_t. Esto permite reutilizar una sola funcion de
 * tarea para los tres canales: solo cambian los indices de canal y la
 * prioridad asignada en xTaskCreate().
 *
 * Flujo de cada tarea:
 *   1. Castear pvParameters a task_params_t*.
 *   2. Leer el ADC del canal configurado.
 *   3. Escalar el valor a duty de 8 bits.
 *   4. Escribir el duty al LED del canal configurado.
 *   5. Imprimir estado por UART cada N ciclos (para no saturar el puerto).
 *   6. Bloquear con vTaskDelay() durante el periodo configurado.
 *
 * Plataforma : ESP32 / FreeRTOS (ESP-IDF 5.x - 6.x)
 * Estandar   : C99
 */

#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"   /* esp_get_free_heap_size() */

/* ------------------------------------------------------------------ */
/*  Configuracion de tareas                                            */
/* ------------------------------------------------------------------ */

/** Periodo de muestreo de cada tarea en milisegundos. */
#define TASK_PERIOD_MS          50U

/** Tamano de stack asignado a cada tarea (en words de 4 bytes). */
#define TASK_STACK_SIZE_WORDS   4096U

/** Cada cuantos ciclos se imprime el estado por UART (reduce carga). */
#define TASK_LOG_EVERY_N        20U

/* ------------------------------------------------------------------ */
/*  Tipos publicos                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief  Parametros pasados a cada tarea mediante pvParameters.
 *
 * Permite instanciar la misma funcion de tarea tres veces con
 * configuraciones distintas, evitando codigo duplicado.
 */
typedef struct {
    uint8_t     adc_channel;   /**< Indice del canal ADC  (0, 1 o 2) */
    uint8_t     led_channel;   /**< Indice del canal LED  (0, 1 o 2) */
    const char *name;          /**< Etiqueta para mensajes UART       */
} task_params_t;

/* ------------------------------------------------------------------ */
/*  API publica                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief  Funcion de tarea FreeRTOS para leer un potenciometro y
 *         ajustar el brillo del LED correspondiente.
 *
 * @param pvParameters  Puntero a task_params_t con la configuracion
 *                      del canal. El bloque de parametros debe tener
 *                      duracion estatica o en heap (no en stack de main).
 *
 * @note   La tarea no termina; corre en bucle infinito con vTaskDelay().
 */
void vTaskPotLED(void *pvParameters);

/**
 * @brief  Crea las tres instancias de vTaskPotLED con sus parametros.
 *
 * Centraliza la llamada a xTaskCreate() para mantener main() limpio.
 * Las prioridades son 1, 2 y 3 para los canales 0, 1 y 2 respectivamente.
 */
void tasks_create_all(void);

#endif /* TASKS_H */
