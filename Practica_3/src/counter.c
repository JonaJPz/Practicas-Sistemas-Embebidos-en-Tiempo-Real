/**
 * @file    counter.c
 * @brief   Implementacion de la tarea contadora BCD.
 *
 * La tarea lee el system_state para conocer la velocidad, luego llama
 * system_state_step() para avanzar el contador si el sistema esta running,
 * y finalmente actualiza el display con el valor actual.
 *
 * Si el sistema esta pausado (running = false), system_state_step() no
 * modifica el valor — el display mantiene el ultimo digito visible.
 *
 * La tarea siempre corre (nunca se suspende a si misma); el Task Manager
 * la suspende externamente con vTaskSuspend() al pausar.
 *
 * Plataforma : ESP32 / FreeRTOS
 * Estandar   : C99
 */

#include "counter.h"
#include "system_state.h"
#include "leds.h"
#include "app_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "COUNTER";

void vTaskCounter(void *pvParameters)
{
    /* pvParameters no se usa en esta tarea */
    (void)pvParameters;

    ESP_LOGI(TAG, "Tarea contador iniciada");

    for (;;)
    {
        /* 1. Leer estado actual para obtener velocidad y valor */
        system_state_t st;
        system_state_get(&st);

        /* 2. Determinar el periodo de espera segun velocidad */
        uint32_t period_ms = (st.speed == SPEED_FAST)
                             ? COUNTER_PERIOD_FAST_MS
                             : COUNTER_PERIOD_SLOW_MS;

        /* 3. Avanzar el contador (solo si running — protegido en step) */
        system_state_step();

        /* 4. Leer valor actualizado y mostrarlo en el display */
        system_state_get(&st);
        leds_show_digit(st.value);

        /* 5. Esperar el periodo configurado (cede CPU al planificador) */
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}
