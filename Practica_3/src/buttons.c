/**
 * @file    buttons.c
 * @brief   Implementacion de lectura de botones con debounce por software.
 *
 * Cada boton tiene su propia tarea FreeRTOS parametrizada con pvParameters.
 * El debounce se implementa esperando BTN_DEBOUNCE_MS despues de detectar
 * el flanco de bajada y verificando que el nivel siga en LOW.
 *
 * Logica de cada boton:
 *   - BTN_START_PAUSE : llama app_tasks_toggle_pause()
 *   - BTN_DIRECTION   : alterna direction (solo si running)
 *   - BTN_SPEED       : alterna speed     (solo si running)
 *
 * Plataforma : ESP32 / FreeRTOS
 * Estandar   : C99
 */

#include "buttons.h"
#include "app_config.h"
#include "app_tasks.h"
#include "system_state.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BTN";

/* ------------------------------------------------------------------ */
/*  Inicializacion                                                     */
/* ------------------------------------------------------------------ */

void buttons_init(void)
{
    /* Configurar los tres botones como entrada con pull-up */
    const gpio_num_t btn_gpios[] = {
        BTN_START_PAUSE, BTN_DIRECTION, BTN_SPEED
    };

    for (uint8_t i = 0U; i < 3U; i++) {
        gpio_config_t io_cfg = {
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE,
            .pin_bit_mask = (1ULL << btn_gpios[i]),
        };
        ESP_ERROR_CHECK(gpio_config(&io_cfg));
    }

    ESP_LOGI(TAG, "Botones inicializados (pull-up, activo LOW)");
}

/* ------------------------------------------------------------------ */
/*  Tarea de boton                                                     */
/* ------------------------------------------------------------------ */

void vTaskButton(void *pvParameters)
{
    /* Obtener configuracion del boton via pvParameters */
    const btn_task_params_t *cfg = (const btn_task_params_t *)pvParameters;

    ESP_LOGI(TAG, "Tarea boton %d en GPIO %d iniciada", cfg->id, cfg->gpio);

    for (;;)
    {
        /* Esperar a que el boton sea presionado (nivel LOW) */
        if (gpio_get_level(cfg->gpio) == 0) {

            /* Debounce: esperar y re-verificar */
            vTaskDelay(pdMS_TO_TICKS(BTN_DEBOUNCE_MS));

            if (gpio_get_level(cfg->gpio) == 0) {

                /* Accion segun el boton identificado */
                switch (cfg->id) {

                    case BTN_ID_START_PAUSE:
                        /* Alterna entre running y pausado */
                        app_tasks_toggle_pause();
                        ESP_LOGI(TAG, "Start/Pause -> %s",
                                 app_tasks_is_paused() ? "PAUSADO" : "CORRIENDO");
                        break;

                    case BTN_ID_DIRECTION:
                        /* Solo actua si el sistema esta corriendo */
                        if (!app_tasks_is_paused()) {
                            system_state_t st;
                            system_state_get(&st);
                            st.direction = (st.direction == DIRECTION_UP)
                                           ? DIRECTION_DOWN
                                           : DIRECTION_UP;
                            system_state_set(&st);
                            ESP_LOGI(TAG, "Direccion -> %s",
                                     st.direction == DIRECTION_UP
                                     ? "ASCENDENTE" : "DESCENDENTE");
                        } else {
                            ESP_LOGI(TAG, "Direccion ignorada (sistema pausado)");
                        }
                        break;

                    case BTN_ID_SPEED:
                        /* Solo actua si el sistema esta corriendo */
                        if (!app_tasks_is_paused()) {
                            system_state_t st;
                            system_state_get(&st);
                            st.speed = (st.speed == SPEED_SLOW)
                                       ? SPEED_FAST
                                       : SPEED_SLOW;
                            system_state_set(&st);
                            ESP_LOGI(TAG, "Velocidad -> %s",
                                     st.speed == SPEED_SLOW ? "500ms" : "250ms");
                        } else {
                            ESP_LOGI(TAG, "Velocidad ignorada (sistema pausado)");
                        }
                        break;

                    default:
                        break;
                }

                /* Esperar a que el boton sea soltado antes de continuar */
                while (gpio_get_level(cfg->gpio) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(10U));
                }
                /* Pequeño delay extra para evitar rebotes al soltar */
                vTaskDelay(pdMS_TO_TICKS(BTN_DEBOUNCE_MS));
            }
        }

        /* Polling cada 10 ms — bajo consumo de CPU */
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}
