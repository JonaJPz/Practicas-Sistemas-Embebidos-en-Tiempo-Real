/**
 * @file    app_tasks.c
 * @brief   Task Manager: crea y controla todas las tareas FreeRTOS.
 *
 * Centraliza los TaskHandle_t de cada tarea para que el resto del
 * sistema pueda controlar el ciclo de vida (suspender/reanudar) sin
 * necesitar handles globales en otros modulos.
 *
 * Tareas creadas:
 *   - vTaskCounter   prioridad 3  — contador BCD + display
 *   - vTaskButton x3 prioridad 2  — un boton cada uno (pvParameters)
 *   - vTaskManager   prioridad 1  — monitoreo y log de estados
 *
 * Plataforma : ESP32 / FreeRTOS
 * Estandar   : C99
 */

#include "app_tasks.h"
#include "app_config.h"
#include "counter.h"
#include "buttons.h"
#include "system_state.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "MANAGER";

/* ------------------------------------------------------------------ */
/*  Handles de tareas                                                  */
/* ------------------------------------------------------------------ */

static TaskHandle_t s_counter_handle   = NULL;
static TaskHandle_t s_btn_handles[3]   = { NULL, NULL, NULL };
static TaskHandle_t s_manager_handle   = NULL;

/* ------------------------------------------------------------------ */
/*  Parametros estaticos de botones (pvParameters)                    */
/* ------------------------------------------------------------------ */

static const btn_task_params_t s_btn_params[3] = {
    { .id = BTN_ID_START_PAUSE, .gpio = BTN_START_PAUSE },
    { .id = BTN_ID_DIRECTION,   .gpio = BTN_DIRECTION   },
    { .id = BTN_ID_SPEED,       .gpio = BTN_SPEED       },
};

/* ------------------------------------------------------------------ */
/*  Control de pausa                                                   */
/* ------------------------------------------------------------------ */

void app_tasks_toggle_pause(void)
{
    if (s_counter_handle == NULL) return;

    system_state_t st;
    system_state_get(&st);

    if (!st.running) {
        /* Estaba pausada: reanudar */
        st.running = true;
        system_state_set(&st);
        vTaskResume(s_counter_handle);
        ESP_LOGI(TAG, "Contador REANUDADO (vTaskResume)");
    } else {
        /* Estaba corriendo: pausar */
        st.running = false;
        system_state_set(&st);
        vTaskSuspend(s_counter_handle);
        ESP_LOGI(TAG, "Contador PAUSADO (vTaskSuspend)");
    }
}

bool app_tasks_is_paused(void)
{
    system_state_t st;
    system_state_get(&st);
    return !st.running;
}

/* ------------------------------------------------------------------ */
/*  Task Manager                                                       */
/* ------------------------------------------------------------------ */

void vTaskManager(void *pvParameters)
{
    (void)pvParameters;

    /* Nombres para el log de estados */
    static const char *state_names[] = {
        "RUNNING", "READY", "BLOCKED", "SUSPENDED", "DELETED", "INVALID"
    };

    ESP_LOGI(TAG, "Task Manager iniciado");

    for (;;)
    {
        /* Reportar estado de las tareas cada 2 segundos */
        vTaskDelay(pdMS_TO_TICKS(2000U));

        eTaskState cs = eTaskGetState(s_counter_handle);
        ESP_LOGI(TAG, "--- Estado de tareas ---");
        ESP_LOGI(TAG, "  vTaskCounter : %s", state_names[cs]);

        for (uint8_t i = 0U; i < 3U; i++) {
            if (s_btn_handles[i] != NULL) {
                eTaskState bs = eTaskGetState(s_btn_handles[i]);
                ESP_LOGI(TAG, "  vTaskButton%u : %s", i, state_names[bs]);
            }
        }

        system_state_t st;
        system_state_get(&st);
        ESP_LOGI(TAG, "  Valor BCD: %u | Dir: %s | Vel: %s | Running: %s",
                 st.value,
                 st.direction == DIRECTION_UP ? "ASC" : "DESC",
                 st.speed == SPEED_SLOW ? "500ms" : "250ms",
                 st.running ? "SI" : "NO");
    }
}

/* ------------------------------------------------------------------ */
/*  Creacion de todas las tareas                                       */
/* ------------------------------------------------------------------ */

void app_tasks_create_all(void)
{
    BaseType_t ret;

    /* --- Tarea contadora (prioridad 3, mayor) --- */
    ret = xTaskCreate(vTaskCounter,
                      "Counter",
                      STACK_COUNTER_WORDS,
                      NULL,
                      3U,
                      &s_counter_handle);
    configASSERT(ret == pdPASS);

    /* Suspender inmediatamente — espera que el usuario presione Start */
    vTaskSuspend(s_counter_handle);
    ESP_LOGI(TAG, "vTaskCounter creada y suspendida (esperando Start)");

    /* --- Tareas de botones (prioridad 2, una por boton) --- */
    const char *btn_names[] = { "BtnStart", "BtnDir", "BtnSpeed" };

    for (uint8_t i = 0U; i < 3U; i++) {
        ret = xTaskCreate(vTaskButton,
                          btn_names[i],
                          STACK_BUTTON_WORDS,
                          (void *)&s_btn_params[i],  /* pvParameters */
                          2U,
                          &s_btn_handles[i]);
        configASSERT(ret == pdPASS);
        ESP_LOGI(TAG, "%s creada (GPIO %d)", btn_names[i], s_btn_params[i].gpio);
    }

    /* --- Task Manager (prioridad 1, menor) --- */
    ret = xTaskCreate(vTaskManager,
                      "Manager",
                      STACK_MANAGER_WORDS,
                      NULL,
                      1U,
                      &s_manager_handle);
    configASSERT(ret == pdPASS);

    ESP_LOGI(TAG, "Todas las tareas creadas correctamente");
}
