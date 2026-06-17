/**
 * @file    main.c
 * @brief   Practica 1 — Sistemas Embebidos en Tiempo Real
 *          Tareas, prioridades, Idle Hook y ciclo LED/Sensor
 *
 * @details Sistema multitarea FreeRTOS sobre ESP32 (ESP-IDF v5.x).
 *          Gestiona dos modos de parpadeo de LED controlados por boton,
 *          lectura de sensor ADC y reporte por UART.
 *h
 *          Hardware utilizado (configuracion del alumno):
 *            GPIO17  — LED        (salida digital)
 *            GPIO5   — Boton      (entrada con PULL-UP interno)
 *            GPIO34  — ADC1 CH6   (entrada analogica, potenciometro 10k)
 *
 * @note    El codigo sigue las reglas aplicables de MISRA-C:2012.
 *          Las desviaciones necesarias por la API de ESP-IDF se documentan
 *          en linea con la etiqueta [MISRA-DEVIATION].
 *
 * @note    [ADC1-RESTRICTION] GPIO34 pertenece a ADC1_CHANNEL_6.
 *          ADC1 permite uso simultaneo con Wi-Fi y Bluetooth (no es conflictivo
 *          a diferencia de ADC2). Para uso exclusivo en practica esta es la
 *          configuracion recomendada.
 *          Ref: ESP32 Technical Reference Manual, seccion 29.3.
 *
 * Plataforma : ESP32 — ESP-IDF v5.x
 * RTOS       : FreeRTOS incluido en ESP-IDF
 */
 
/* ============================================================
   INCLUDES
   Regla MISRA 20.1 — incluir solo cabeceras necesarias, en orden.
   ============================================================ */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
 
/* ============================================================
   CONFIGURACION DE PINES
   Regla MISRA 2.5 — macros deben usarse donde se definen.
   Se usa 'U' suffix en literales enteros sin signo (MISRA 7.2).
   ============================================================ */
#define GPIO_LED_PIN     (17U)           /**< Pin de salida para el LED             */
#define GPIO_BUTTON_PIN  (5U)            /**< Pin de entrada para el boton          */
 
/* [ADC1-RESTRICTION] GPIO34 = ADC1_CHANNEL_6.                                     */
#define ADC_GPIO_PIN     (34U)           /**< Pin analogico: GPIO34                 */
#define ADC_INPUT_CH (ADC_CHANNEL_6)     /**< Canal ADC1_6 — GPIO34 en ESP32        */
 
/* ============================================================
   PARAMETROS DE TIMING
   Todos los tiempos en milisegundos para legibilidad.
   ============================================================ */
#define LED_FAST_HALF_MS   (100U)   /**< Semiperiodo en modo rapido: 100 ms       */
#define LED_SLOW_HALF_MS   (500U)   /**< Semiperiodo en modo lento:  500 ms       */
#define SENSOR_PERIOD_MS   (300U)   /**< Periodo de lectura ADC:     300 ms       */
#define SLOW_TIMEOUT_MS    (5000U)  /**< Timeout antes de volver a modo rapido    */
#define BUTTON_POLL_MS     (20U)    /**< Periodo de polling del boton:  20 ms     */
#define IDLE_SPIN_DELAY    (10U)    /**< Retardo de cortesia en bucle de espera   */
 
/* Constantes del ADC */
#define ADC_VREF_MV        (3300U)  /**< Tension de referencia del ADC en mV      */
#define ADC_FULL_SCALE     (4095U)  /**< Valor maximo para 12 bits (2^12 - 1)     */
 
/* ============================================================
   VARIABLES GLOBALES COMPARTIDAS
   Regla MISRA 8.4 — declaracion visible antes de uso.
   'volatile' es obligatorio: el compilador no puede optimizar
   accesos a variables que otras tareas pueden modificar en
   cualquier momento (MISRA 8.3 — tipo completo en declaracion).
   ============================================================ */
 
/** Indica que el sistema esta en modo rapido (true) o lento (false). */
static volatile bool g_mode_fast = true;
 
/** Senaliza que el boton fue presionado; lo activa vTaskMonitor. */
static volatile bool g_btn_pressed = false;
 
/** Habilita el reporte del sensor ADC; lo activa vTaskMonitor. */
static volatile bool g_sensor_on = false;
 
/* Handle de tarea — necesario para vTaskResume desde otra tarea.
   [MISRA-DEVIATION 8.6] Valor inicial NULL es seguro aqui porque
   la tarea se crea antes de que cualquier otra la pueda reanudar. */
static TaskHandle_t g_handle_fast = NULL;
 
/** Handle del periferico ADC en modo oneshot. */
static adc_oneshot_unit_handle_t g_adc_handle = NULL;
 
/* ============================================================
   PROTOTIPOS DE FUNCIONES PRIVADAS
   Regla MISRA 8.2 — prototipo visible antes de la definicion.
   ============================================================ */
static void task_led_fast(void *param);
static void task_led_slow(void *param);
static void task_sensor(void *param);
static void task_monitor(void *param);
static void init_gpio(void);
static void init_adc(void);
 
/* ============================================================
   TAREA 1: vTaskLedRapido — Prioridad 1 (la mas baja de usuario)
   ============================================================ */
static void task_led_fast(void *param)
{
    (void)param;
 
    while (1)
    {
        if (g_btn_pressed == true)
        {
            printf("[LED_R] Boton detectado -> suspendiendo modo RAPIDO\n");
            vTaskSuspend(NULL);
        }
 
        (void)gpio_set_level(GPIO_LED_PIN, 1U);
        printf("[LED_R] ON\n");
        vTaskDelay(pdMS_TO_TICKS(LED_FAST_HALF_MS));
 
        (void)gpio_set_level(GPIO_LED_PIN, 0U);
        printf("[LED_R] OFF\n");
        vTaskDelay(pdMS_TO_TICKS(LED_FAST_HALF_MS));
    }
}
 
/* ============================================================
   TAREA 2: vTaskLedLento — Prioridad 2
   ============================================================ */
static void task_led_slow(void *param)
{
    (void)param;
 
    TickType_t tick_start;
    uint32_t   elapsed_ms;
    uint32_t   remaining_ms;
 
    while (1)
    {
        if (g_mode_fast == false)
        {
            tick_start = xTaskGetTickCount();
 
            while (g_mode_fast == false)
            {
                elapsed_ms = (uint32_t)((xTaskGetTickCount() - tick_start)
                             * (TickType_t)portTICK_PERIOD_MS);
 
                if (elapsed_ms >= SLOW_TIMEOUT_MS)
                {
                    printf("[LED_L] Timeout 5s -> regresando a modo RAPIDO\n");
                    g_sensor_on   = false;
                    g_btn_pressed = false;
                    g_mode_fast   = true;
                    (void)gpio_set_level(GPIO_LED_PIN, 0U);
                    vTaskResume(g_handle_fast);
                    break;
                }
 
                remaining_ms = SLOW_TIMEOUT_MS - elapsed_ms;
 
                (void)gpio_set_level(GPIO_LED_PIN, 1U);
                printf("[LED_L] ON  t_restante: %lu ms\n", remaining_ms);
                vTaskDelay(pdMS_TO_TICKS(LED_SLOW_HALF_MS));
 
                elapsed_ms = (uint32_t)((xTaskGetTickCount() - tick_start)
                             * (TickType_t)portTICK_PERIOD_MS);
 
                if (elapsed_ms >= SLOW_TIMEOUT_MS)
                {
                    printf("[LED_L] Timeout 5s -> regresando a modo RAPIDO\n");
                    g_sensor_on   = false;
                    g_btn_pressed = false;
                    g_mode_fast   = true;
                    (void)gpio_set_level(GPIO_LED_PIN, 0U);
                    vTaskResume(g_handle_fast);
                    break;
                }
 
                remaining_ms = SLOW_TIMEOUT_MS - elapsed_ms;
 
                (void)gpio_set_level(GPIO_LED_PIN, 0U);
                printf("[LED_L] OFF t_restante: %lu ms\n", remaining_ms);
                vTaskDelay(pdMS_TO_TICKS(LED_SLOW_HALF_MS));
            }
        }
 
        vTaskDelay(pdMS_TO_TICKS(IDLE_SPIN_DELAY));
    }
}
 
/* ============================================================
   TAREA 3: vTaskSensor — Prioridad 3
   ============================================================ */
static void task_sensor(void *param)
{
    (void)param;
 
    int       raw_val    = 0;
    uint32_t  voltage_mv = 0U;
    esp_err_t status;
 
    while (1)
    {
        if (g_sensor_on == true)
        {
            status = adc_oneshot_read(g_adc_handle, ADC_INPUT_CH, &raw_val);
 
            if (status == ESP_OK)
            {
                voltage_mv = ((uint32_t)raw_val * ADC_VREF_MV) / ADC_FULL_SCALE;
 
                printf("[SENS] ADC raw: %d  Voltaje: %lu.%02lu V\n",
                       raw_val,
                       voltage_mv / 1000UL,
                       (voltage_mv % 1000UL) / 10UL);
            }
            else
            {
                /* ESP_ERR_TIMEOUT aqui puede indicar conflicto con radio en ADC2.
                   Con ADC1 (GPIO34) esto no ocurre. */
                printf("[SENS] Error al leer ADC (codigo: %d)\n", (int)status);
            }
        }
 
        vTaskDelay(pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}
 
/* ============================================================
   TAREA 4: vTaskMonitor — Prioridad 4 (la mas alta de usuario)
   ============================================================ */
static void task_monitor(void *param)
{
    (void)param;
 
    bool btn_prev    = true;
    bool btn_current = true;
 
    while (1)
    {
        /* [MISRA-DEVIATION 10.1] gpio_get_level devuelve int;
           lo convertimos explicitamente a bool. */
        btn_current = (gpio_get_level(GPIO_BUTTON_PIN) != 0);
 
        if ((btn_prev == true) && (btn_current == false))
        {
            printf("[MON] *** BOTON PRESIONADO ***\n");
            g_btn_pressed = true;
            g_mode_fast   = false;
            g_sensor_on   = true;
        }
 
        btn_prev = btn_current;
 
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}
 
/* ============================================================
   IDLE HOOK
   ============================================================ */
void vApplicationIdleHook(void)
{
    static uint32_t idle_count = 0U;
 
    idle_count++;
 
    if (idle_count >= 500000U)
    {
        idle_count = 0U;
        printf("[IDLE] CPU libre — esperando evento de boton\n");
    }
}
 
/* ============================================================
   INICIALIZACION DEL ADC — ESP-IDF v5.x oneshot API
   
   GPIO34 = ADC1_CHANNEL_6:
     - unit_id : ADC_UNIT_1  (no conflicto con Wi-Fi / Bluetooth)
     - atten   : ADC_ATTEN_DB_12  (0..3.3V)
   ============================================================ */
static void init_adc(void)
{
    esp_err_t status;
 
    /* [ADC1-RESTRICTION] Se inicializa ADC_UNIT_1 porque GPIO34 = ADC1_CH6.
       ADC1 es segura para usar con Wi-Fi y Bluetooth (no hay restriction de hardware). */
    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,              /* ← ADC1 para GPIO34             */
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
 
    status = adc_oneshot_new_unit(&unit_cfg, &g_adc_handle);
    if (status != ESP_OK)
    {
        printf("[INIT] Fallo al inicializar ADC1 (codigo: %d)\n", (int)status);
        return;
    }
 
    const adc_oneshot_chan_cfg_t ch_cfg = {
        .bitwidth = ADC_BITWIDTH_12,         /* 12 bits -> 0..4095              */
        .atten    = ADC_ATTEN_DB_12,         /* 0..3.3V — rango completo        */
    };
 
    status = adc_oneshot_config_channel(g_adc_handle, ADC_INPUT_CH, &ch_cfg);
    if (status != ESP_OK)
    {
        printf("[INIT] Fallo al configurar ADC1 CH6 / GPIO34 (codigo: %d)\n", (int)status);
    }
    else
    {
        printf("[INIT] ADC1 CH6 (GPIO34) configurado correctamente\n");
    }
}
 
/* ============================================================
   INICIALIZACION DE GPIO
   ============================================================ */
static void init_gpio(void)
{
    esp_err_t status;
 
    const gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << GPIO_LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
 
    status = gpio_config(&led_cfg);
    if (status != ESP_OK)
    {
        printf("[INIT] Fallo al configurar GPIO del LED\n");
    }
 
    (void)gpio_set_level(GPIO_LED_PIN, 0U);
 
    const gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << GPIO_BUTTON_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
 
    status = gpio_config(&btn_cfg);
    if (status != ESP_OK)
    {
        printf("[INIT] Fallo al configurar GPIO del boton\n");
    }
}
 
/* ============================================================
   PUNTO DE ENTRADA: app_main
   ============================================================ */
void app_main(void)
{
    BaseType_t created;
 
    printf("\n=== Practica 1 — FreeRTOS en ESP32 ===\n");
    printf("Presiona el boton (GPIO%u) para activar modo LENTO\n", GPIO_BUTTON_PIN);
    printf("Timeout en modo lento: %u segundos\n",
           (unsigned int)(SLOW_TIMEOUT_MS / 1000U));
    printf("ADC: GPIO34 = ADC1_CHANNEL_6\n\n");
 
    init_gpio();
    init_adc();
 
    created = xTaskCreate(task_led_fast,
                          "LED_Fast",
                          2048U,
                          NULL,
                          1U,
                          &g_handle_fast);
    if (created != pdPASS)
    {
        printf("[INIT] Error al crear tarea LED_Fast\n");
    }
 
    created = xTaskCreate(task_led_slow,
                          "LED_Slow",
                          2048U,
                          NULL,
                          2U,
                          NULL);
    if (created != pdPASS)
    {
        printf("[INIT] Error al crear tarea LED_Slow\n");
    }
 
    created = xTaskCreate(task_sensor,
                          "Sensor",
                          2048U,
                          NULL,
                          3U,
                          NULL);
    if (created != pdPASS)
    {
        printf("[INIT] Error al crear tarea Sensor\n");
    }
 
    created = xTaskCreate(task_monitor,
                          "Monitor",
                          3072U,
                          NULL,
                          4U,
                          NULL);
    if (created != pdPASS)
    {
        printf("[INIT] Error al crear tarea Monitor\n");
    }
 
    printf("[INIT] Todas las tareas creadas. Scheduler activo.\n");
    printf("[INIT] Prefijos esperados: [LED_R] [LED_L] [SENS] [MON] [IDLE]\n");
}