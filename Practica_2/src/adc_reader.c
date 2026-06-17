/**
 * @file    adc_reader.c
 * @brief   Implementacion de lectura ADC oneshot para tres potenciometros.
 *
 * Utiliza la API adc_oneshot de ESP-IDF 5.x/6.x (nueva API; no usa
 * adc1_get_raw() de la API legacy). El handle del ADC se guarda como
 * variable estatica para ser reutilizado por las tareas.
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x
 * Estandar   : C99
 */

#include "adc_reader.h"
#include "esp_log.h"

/* ------------------------------------------------------------------ */
/*  Constantes internas                                                */
/* ------------------------------------------------------------------ */

static const char *TAG = "ADC_READER";

/* ------------------------------------------------------------------ */
/*  Estado interno                                                     */
/* ------------------------------------------------------------------ */

/** Handle del modulo ADC1 oneshot (compartido entre los tres canales). */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

/** Tabla: indice logico -> canal fisico ADC1. */
static const adc_channel_t adc_channel_map[ADC_CHANNEL_COUNT] = {
    ADC_CH_POT0,   /* Potenciometro 0 -> GPIO 36 */
    ADC_CH_POT1,   /* Potenciometro 1 -> GPIO 39 */
    ADC_CH_POT2,   /* Potenciometro 2 -> GPIO 34 */
};

/* ------------------------------------------------------------------ */
/*  Implementacion                                                     */
/* ------------------------------------------------------------------ */

void adc_reader_init(void)
{
    /* Crear unidad ADC1 en modo oneshot */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &s_adc_handle));

    /* Configurar atenuacion y resolucion por canal */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,       /* ~0-3.3 V */
        .bitwidth = ADC_BITWIDTH_DEFAULT,  /* 12 bits en ESP32 */
    };

    for (uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(
            s_adc_handle, adc_channel_map[i], &chan_cfg));
    }

    ESP_LOGI(TAG, "ADC inicializado: %u canales, 12 bits, atten 12 dB",
             ADC_CHANNEL_COUNT);
}

uint16_t adc_reader_get_raw(uint8_t channel)
{
    if (channel >= ADC_CHANNEL_COUNT || s_adc_handle == NULL) {
        return 0U;
    }

    int raw = 0;
    /* adc_oneshot_read devuelve ESP_OK si la lectura es valida */
    if (adc_oneshot_read(s_adc_handle, adc_channel_map[channel], &raw)
            == ESP_OK) {
        return (uint16_t)raw;
    }

    return 0U;
}