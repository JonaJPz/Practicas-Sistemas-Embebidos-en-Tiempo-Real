/**
 * @file    app_tasks.h
 * @brief   Interfaz del Task Manager: creacion y control de tareas.
 *
 * El Task Manager centraliza los handles de todas las tareas y
 * expone funciones para pausar/reanudar el contador desde las
 * tareas de botones, sin que estas necesiten conocer los handles.
 *
 * Plataforma : ESP32 / FreeRTOS
 * Estandar   : C99
 */

#ifndef APP_TASKS_H
#define APP_TASKS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ------------------------------------------------------------------ */
/*  API publica                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Crea todas las tareas del sistema y el Task Manager.
 *
 * Debe llamarse una sola vez desde app_main().
 */
void app_tasks_create_all(void);

/**
 * @brief Alterna entre pausar y reanudar la tarea contadora.
 *
 * Llamado por la tarea del boton Start/Pause.
 * Usa vTaskSuspend() / vTaskResume() sobre el handle del contador.
 */
void app_tasks_toggle_pause(void);

/**
 * @brief Retorna el estado actual de pausa del contador.
 *
 * @return true si el contador esta suspendido (pausado).
 */
bool app_tasks_is_paused(void);

/**
 * @brief Tarea FreeRTOS del Task Manager.
 *
 * Monitorea y reporta el estado de las tareas por UART periodicamente.
 *
 * @param pvParameters  No utilizado (NULL).
 */
void vTaskManager(void *pvParameters);

#endif /* APP_TASKS_H */
