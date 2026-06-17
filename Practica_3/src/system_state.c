/**
 * @file    system_state.c
 * @brief   Implementacion del estado compartido del sistema.
 *
 * El mutex garantiza acceso exclusivo desde multiples tareas FreeRTOS.
 *
 * Plataforma : ESP32 / FreeRTOS
 * Estandar   : C99
 */

#include "system_state.h"
#include "app_config.h"
#include "esp_log.h"

static const char *TAG = "STATE";

/* ------------------------------------------------------------------ */
/*  Estado interno                                                     */
/* ------------------------------------------------------------------ */

/** Estado global del sistema. */
static system_state_t s_state = {
    .value     = 0U,
    .direction = DIRECTION_UP,
    .speed     = SPEED_SLOW,
    .running   = false,   /* Inicia pausado hasta que el usuario presione Start */
};

/** Mutex para acceso thread-safe al estado. */
static SemaphoreHandle_t s_mutex = NULL;

/* ------------------------------------------------------------------ */
/*  Implementacion                                                     */
/* ------------------------------------------------------------------ */

void system_state_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    configASSERT(s_mutex != NULL);
    ESP_LOGI(TAG, "Estado del sistema inicializado");
}

void system_state_get(system_state_t *out)
{
    configASSERT(out != NULL);
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_state;
    xSemaphoreGive(s_mutex);
}

void system_state_set(const system_state_t *in)
{
    configASSERT(in != NULL);
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_state = *in;
    xSemaphoreGive(s_mutex);
}

void system_state_step(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);

    if (s_state.running) {
        if (s_state.direction == DIRECTION_UP) {
            /* Ascendente con wrap-around 9 -> 0 */
            s_state.value = (s_state.value >= COUNTER_MAX)
                            ? COUNTER_MIN
                            : (s_state.value + 1U);
        } else {
            /* Descendente con wrap-around 0 -> 9 */
            s_state.value = (s_state.value <= COUNTER_MIN)
                            ? COUNTER_MAX
                            : (s_state.value - 1U);
        }
    }

    xSemaphoreGive(s_mutex);
}
