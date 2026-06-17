/**
 * @file    tasks.c
 * @brief   Implementacion de las tareas FreeRTOS para control PWM por ADC.
 *
 * Una sola funcion de tarea (vTaskPotLED) sirve a los tres canales.
 * La diferenciacion se logra exclusivamente a traves del puntero
 * pvParameters, que apunta a un task_params_t estatico distinto por canal.
 *
 * Ciclo de cada tarea:
 *   - Lee el ADC del canal asignado.
 *   - Escala la lectura (12 bits) al ciclo de trabajo PWM (8 bits).
 *   - Actualiza el LED del canal asignado.
 *   - Cada TASK_LOG_EVERY_N iteraciones imprime estado por UART.
 *   - Se bloquea TASK_PERIOD_MS milisegundos (libera el procesador).
 *
 * MISRA-C:2012 notas de cumplimiento:
 *   - Rule 11.5: La conversion void* -> task_params_t* se documenta
 *     explicitamente; es el mecanismo oficial de FreeRTOS pvParameters.
 *   - Rule 15.5: La funcion vTaskPotLED tiene un unico punto de salida
 *     conceptual (el bucle infinito no termina en condiciones normales).
 *   - Rule  8.9: Las variables con uso estrictamente local al modulo se
 *     declaran static.
 *   - Rule 14.4: La condicion del bucle infinito usa la macro portabilizada
 *     de FreeRTOS en lugar de un literal numerico.
 *
 * Plataforma : ESP32 / FreeRTOS (ESP-IDF 5.x - 6.x)
 * Estandar   : C99
 */

#include "tasks.h"
#include "adc_reader.h"
#include "leds.h"
#include "esp_log.h"

/* ------------------------------------------------------------------
 *  Etiqueta de log para este modulo
 * ------------------------------------------------------------------ */

static const char *TAG = "TASKS";  /**< Tag UART para ESP_LOGx */

/* ------------------------------------------------------------------
 *  Bloques de parametros estaticos (uno por canal)
 *
 *  Se declaran static para garantizar que su vida util supera la del
 *  stack de tasks_create_all(). Las tareas leen estos datos durante
 *  toda su ejecucion. (MISRA-C:2012 Rule 18.1: never use a pointer
 *  to an object outside its lifetime.)
 * ------------------------------------------------------------------ */

static const task_params_t params_ch0 = {
    .adc_channel = 0U,
    .led_channel = 0U,
    .name        = "POT0-LED0",
};

static const task_params_t params_ch1 = {
    .adc_channel = 1U,
    .led_channel = 1U,
    .name        = "POT1-LED1",
};

static const task_params_t params_ch2 = {
    .adc_channel = 2U,
    .led_channel = 2U,
    .name        = "POT2-LED2",
};

/* ------------------------------------------------------------------
 *  Handles de tarea (necesarios para uxTaskGetStackHighWaterMark)
 * ------------------------------------------------------------------ */

/** Arreglo de handles; inicializado a NULL segun MISRA Rule 9.3. */
static TaskHandle_t s_task_handles[3] = { NULL, NULL, NULL };

/* ------------------------------------------------------------------
 *  Funcion de tarea
 * ------------------------------------------------------------------ */

/**
 * @brief  Tarea FreeRTOS: lee un potenciometro y actualiza un LED.
 *
 * @param pvParameters  Debe apuntar a un task_params_t valido y estatico.
 *                      Conversion void* -> const task_params_t* conforme
 *                      al mecanismo oficial de FreeRTOS (MISRA Rule 11.5,
 *                      desviacion documentada y justificada por la API).
 */
void vTaskPotLED(void *pvParameters)  /* firma FIJA segun enunciado */
{
    /* --- Obtener configuracion del canal via pvParameters --- */
    /* MISRA Rule 11.5: cast de void* a tipo objeto; justificado porque
     * pvParameters es el mecanismo estandar de FreeRTOS para parametrizar
     * tareas y el puntero siempre apunta a un task_params_t estatico. */
    const task_params_t *cfg = (const task_params_t *)pvParameters;

    uint32_t cycle_count = 0U;   /* Contador de ciclos para log periodico */

    ESP_LOGI(TAG, "[%s] Tarea iniciada (ADC ch%u -> LED ch%u)",
             cfg->name,
             (unsigned int)cfg->adc_channel,
             (unsigned int)cfg->led_channel);

    /* MISRA Rule 14.4: condicion del bucle a traves de macro FreeRTOS. */
    for (;;)
    {
        /* 1. Leer potenciometro: valor raw de 12 bits [0, 4095] */
        uint16_t raw  = adc_reader_get_raw(cfg->adc_channel);

        /* 2. Escalar a duty PWM de 8 bits [0, 255] */
        uint8_t  duty = adc_reader_to_duty(raw);

        /* 3. Aplicar brillo al LED correspondiente */
        leds_set_duty((led_channel_t)cfg->led_channel, duty);

        /* 4. Incrementar contador de ciclos (sin desbordamiento logico
         *    relevante: al llegar a UINT32_MAX vuelve a 0, lo que solo
         *    retrasa un log, sin efecto en la funcionalidad). */
        cycle_count++;

        /* 5. Imprimir estado periodicamente para no saturar UART.
         *    Se reportan: etiqueta, raw, duty, heap libre y watermark
         *    de stack para detectar fugas y ajustar tamano de stack. */
        if ((cycle_count % TASK_LOG_EVERY_N) == 0U)
        {
            UBaseType_t wm = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGI(TAG, "[%s] raw=%4u duty=%3u heap=%lu wm=%u",
                     cfg->name,
                     (unsigned int)raw,
                     (unsigned int)duty,
                     (unsigned long)esp_get_free_heap_size(),
                     (unsigned int)wm);
        }

        /* 6. Bloquear la tarea durante el periodo configurado.
         *    vTaskDelay() cede el procesador al planificador, que puede
         *    ejecutar otras tareas mientras esta permanece bloqueada.
         *    Usar pdMS_TO_TICKS() garantiza independencia de la frecuencia
         *    del tick (configTICK_RATE_HZ). */
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
    }

    /* Nunca se llega aqui; se incluye por buena practica de programacion
     * y para que los analizadores estaticos no reporten falta de return. */
    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------
 *  Creacion de las tres instancias
 * ------------------------------------------------------------------ */

/**
 * @brief  Crea las tres tareas vTaskPotLED con prioridades 1, 2 y 3.
 *
 * Los bloques de parametros son variables static del modulo, por lo que
 * persisten despues de que esta funcion retorna (MISRA Rule 18.1).
 *
 * xTaskCreate() devuelve pdPASS (1) si la creacion fue exitosa.
 * configASSERT() detiene el sistema en debug si falla (MISRA Rule 15.3:
 * manejo explicito del valor de retorno de funciones de la API).
 */
void tasks_create_all(void)
{
    BaseType_t ret;

    /* --- Canal 0: prioridad 1 (menor) --- */
    /* xTaskCreate: crea una tarea FreeRTOS y la agrega a la lista ready. */
    ret = xTaskCreate(vTaskPotLED,
                      "PotLED_0",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch0,   /* pvParameters -> params_ch0 */
                      1U,
                      &s_task_handles[0]);
    configASSERT(ret == pdPASS);

    /* --- Canal 1: prioridad 2 --- */
    ret = xTaskCreate(vTaskPotLED,
                      "PotLED_1",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch1,   /* pvParameters -> params_ch1 */
                      2U,
                      &s_task_handles[1]);
    configASSERT(ret == pdPASS);

    /* --- Canal 2: prioridad 3 (mayor) --- */
    ret = xTaskCreate(vTaskPotLED,
                      "PotLED_2",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch2,   /* pvParameters -> params_ch2 */
                      3U,
                      &s_task_handles[2]);
    configASSERT(ret == pdPASS);

    ESP_LOGI(TAG, "Tres tareas creadas correctamente");
}
