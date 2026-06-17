/**
 * @file    leds.c
 * @brief   Implementacion del display de 7 segmentos (catodo comun).
 *
 * Tabla de segmentos para digitos 0-9.
 * Catodo comun: bit en 1 = segmento encendido (GPIO en HIGH).
 *
 * Orden de bits: { a, b, c, d, e, f, g }
 *
 *
 * Plataforma : ESP32 / ESP-IDF 5.x-6.x
 * Estandar   : C99
 */

#include "leds.h"
#include "app_config.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "LEDS";

/* ------------------------------------------------------------------ */
/*  Tabla de segmentos para digitos 0-9                                */
/*  Indice: { SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G }     */
/* ------------------------------------------------------------------ */

/** Cada fila es un digito. Cada columna es un segmento (a-g). */
static const uint8_t seg_table[10U][SEG_COUNT] = {
    /* a  b  c  d  e  f  g */
    {  1, 1, 1, 1, 1, 1, 0 },  /* 0 */
    {  0, 1, 1, 0, 0, 0, 0 },  /* 1 */
    {  1, 1, 0, 1, 1, 0, 1 },  /* 2 */
    {  1, 1, 1, 1, 0, 0, 1 },  /* 3 */
    {  0, 1, 1, 0, 0, 1, 1 },  /* 4 */
    {  1, 0, 1, 1, 0, 1, 1 },  /* 5 */
    {  1, 0, 1, 1, 1, 1, 1 },  /* 6 */
    {  1, 1, 1, 0, 0, 0, 0 },  /* 7 */
    {  1, 1, 1, 1, 1, 1, 1 },  /* 8 */
    {  1, 1, 1, 1, 0, 1, 1 },  /* 9 */
};

/** Arreglo de GPIOs en orden a-g para iterar facilmente. */
static const gpio_num_t seg_gpios[SEG_COUNT] = {
    SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G
};

/* ------------------------------------------------------------------ */
/*  Implementacion                                                     */
/* ------------------------------------------------------------------ */

void leds_init(void)
{
    gpio_config_t io_cfg = {
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << SEG_A) | (1ULL << SEG_B) |
                        (1ULL << SEG_C) | (1ULL << SEG_D) |
                        (1ULL << SEG_E) | (1ULL << SEG_F) |
                        (1ULL << SEG_G),
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));
    leds_clear();
    ESP_LOGI(TAG, "Display 7 segmentos inicializado (catodo comun)");
}

void leds_show_digit(uint8_t digit)
{
    if (digit > 9U) {
        leds_clear();
        return;
    }

    for (uint8_t i = 0U; i < SEG_COUNT; i++) {
        gpio_set_level(seg_gpios[i], seg_table[digit][i]);
    }
}

void leds_clear(void)
{
    for (uint8_t i = 0U; i < SEG_COUNT; i++) {
        gpio_set_level(seg_gpios[i], 0);
    }
}
