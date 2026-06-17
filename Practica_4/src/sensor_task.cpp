#include "sensor_task.hpp"
#include "app_config.hpp"
#include "app_context.hpp"
#include "messages.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

namespace App
{
    static const char *TAG = "SENSOR";

    static uint16_t median_u16(uint16_t *data, uint8_t n)
    {
        for (uint8_t i = 0; i < n - 1; i++)
        {
            for (uint8_t j = 0; j < n - i - 1; j++)
            {
                if (data[j] > data[j + 1])
                {
                    uint16_t tmp = data[j];
                    data[j] = data[j + 1];
                    data[j + 1] = tmp;
                }
            }
        }

        return data[n / 2];
    }

    static uint8_t target_from_ldr(uint16_t filtered)
    {
        if (filtered >= AppConfig::LDR_THRESHOLD_HIGH)
        {
            return AppConfig::SERVO_ANGLE_LIGHT;
        }

        if (filtered <= AppConfig::LDR_THRESHOLD_LOW)
        {
            return AppConfig::SERVO_ANGLE_DARK;
        }

        return AppConfig::SERVO_ANGLE_DARK;
    }

    void SensorTask::run(void *pvParameters)
    {
        SensorTaskConfig *cfg = static_cast<SensorTaskConfig *>(pvParameters);

        adc_oneshot_unit_handle_t adc_handle;

        adc_oneshot_unit_init_cfg_t init_cfg = {};
        init_cfg.unit_id = cfg->unit_id;
        init_cfg.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
        init_cfg.ulp_mode = ADC_ULP_MODE_DISABLE;

        adc_oneshot_new_unit(&init_cfg, &adc_handle);

        adc_oneshot_chan_cfg_t chan_cfg = {};
        chan_cfg.atten = ADC_ATTEN_DB_12;
        chan_cfg.bitwidth = ADC_BITWIDTH_12;

        adc_oneshot_config_channel(
            adc_handle,
            cfg->channel,
            &chan_cfg
        );

        ESP_LOGI(TAG, "%s iniciado", cfg->name);

        while (true)
        {
            uint16_t samples[16];

            uint8_t n = cfg->filter_window;

            if (n > 16)
            {
                n = 16;
            }

            uint16_t raw_last = 0;

            for (uint8_t i = 0; i < n; i++)
            {
                int raw = 0;

                adc_oneshot_read(
                    adc_handle,
                    cfg->channel,
                    &raw
                );

                samples[i] = (uint16_t)raw;
                raw_last = (uint16_t)raw;

                vTaskDelay(pdMS_TO_TICKS(20));
            }

            uint16_t filtered = median_u16(samples, n);
            uint8_t target = target_from_ldr(filtered);

            SensorMsg msg = {};
            msg.raw = raw_last;
            msg.filtered = filtered;
            msg.target_angle = target;

            xQueueOverwrite(g_queues.sensor, &msg);

            ESP_LOGI(TAG,
                     "raw=%u filtered=%u target=%u",
                     msg.raw,
                     msg.filtered,
                     msg.target_angle);

            vTaskDelay(pdMS_TO_TICKS(cfg->period_ms));
        }
    }
}