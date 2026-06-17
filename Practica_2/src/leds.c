/**
 * @file    leds.c
 * @brief   Implementacion del control PWM para tres LEDs via LEDC (ESP32).
 *
 * Utiliza un unico timer LEDC de alta velocidad compartido entre los tres
 * canales. La resolucion de 8 bits permite 256 niveles de brillo, suficiente
 * para representar la lectura ADC de 12 bits escalada a ese rango.
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x
 * Estandar   : C99
 */

#include "leds.h"
#include "esp_log.h"

/* ------------------------------------------------------------------ */
/*  Constantes internas                                                */
/* ------------------------------------------------------------------ */

static const char *TAG = "LEDS";

/** Timer LEDC compartido por los tres canales. */
#define LED_TIMER   LEDC_TIMER_0
/** Modo de velocidad del periferico LEDC. */
#define LED_MODE    LEDC_HIGH_SPEED_MODE

/* ------------------------------------------------------------------ */
/*  Tabla de configuracion de canales                                  */
/* ------------------------------------------------------------------ */

/** Asocia cada canal logico con su GPIO y canal LEDC fisico. */
static const struct 
{
    gpio_num_t      gpio;
    ledc_channel_t  channel;
    
} 
    
    led_map[LED_CHANNEL_COUNT] = {{ LED_PIN_0, LEDC_CHANNEL_0 },
                                  { LED_PIN_1, LEDC_CHANNEL_1 },
                                  { LED_PIN_2, LEDC_CHANNEL_2 },
                                 };

/* ------------------------------------------------------------------ */
/*  Implementacion                                                     */
/* ------------------------------------------------------------------ */

void leds_init(void)
{
    /* Configurar el timer LEDC compartido */
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LED_MODE,
        .timer_num       = LED_TIMER,
        .duty_resolution = LED_PWM_RESOLUTION,
        .freq_hz         = LED_PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    /* Configurar cada canal LEDC */
    for (uint8_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        ledc_channel_config_t ch_cfg = {
            .speed_mode = LED_MODE,
            .channel    = led_map[i].channel,
            .timer_sel  = LED_TIMER,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = led_map[i].gpio,
            .duty       = 0,   /* Iniciar apagado */
            .hpoint     = 0,
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
    }

    ESP_LOGI(TAG, "Subsistema PWM inicializado (%u canales, %u Hz, 8 bits)",LED_CHANNEL_COUNT, LED_PWM_FREQ_HZ);
}

void leds_set_duty(led_channel_t channel, uint8_t duty)
{
    if (channel >= LED_CHANNEL_COUNT) {
        /* Canal fuera de rango: ignorar silenciosamente */
        return;
    }

    /* Escribir el nuevo ciclo de trabajo y actualizar el periferico */
    ledc_set_duty(LED_MODE, led_map[channel].channel, duty);
    ledc_update_duty(LED_MODE, led_map[channel].channel);
}