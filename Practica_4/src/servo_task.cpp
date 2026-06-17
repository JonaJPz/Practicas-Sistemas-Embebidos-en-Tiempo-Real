#include "servo_task.hpp"
#include "app_context.hpp"
#include "messages.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/ledc.h"
#include "esp_log.h"

namespace App
{
    static const char *TAG = "SERVO";

    static uint32_t angle_to_duty(uint8_t angle, const ServoTaskConfig *cfg)
    {
        uint32_t max_duty = (1UL << cfg->resolution) - 1UL;

        uint32_t pulse_us =
            cfg->min_us +
            ((cfg->max_us - cfg->min_us) * angle) / 180;

        uint32_t period_us = 1000000UL / cfg->freq_hz;

        uint32_t duty =
            (pulse_us * max_duty) / period_us;

        return duty;
    }

    static void servo_write_angle(uint8_t angle, const ServoTaskConfig *cfg)
    {
        uint32_t duty = angle_to_duty(angle, cfg);

        ledc_set_duty(cfg->mode, cfg->channel, duty);
        ledc_update_duty(cfg->mode, cfg->channel);
    }

    void ServoTask::run(void *pvParameters)
    {
        auto *cfg = static_cast<ServoTaskConfig *>(pvParameters);

        ledc_timer_config_t timer_cfg = {};
        timer_cfg.speed_mode = cfg->mode;
        timer_cfg.duty_resolution = cfg->resolution;
        timer_cfg.timer_num = cfg->timer;
        timer_cfg.freq_hz = cfg->freq_hz;
        timer_cfg.clk_cfg = LEDC_AUTO_CLK;

        ledc_timer_config(&timer_cfg);

        ledc_channel_config_t channel_cfg = {};
        channel_cfg.gpio_num = cfg->gpio;
        channel_cfg.speed_mode = cfg->mode;
        channel_cfg.channel = cfg->channel;
        channel_cfg.intr_type = LEDC_INTR_DISABLE;
        channel_cfg.timer_sel = cfg->timer;
        channel_cfg.duty = 0;
        channel_cfg.hpoint = 0;

        ledc_channel_config(&channel_cfg);

        uint8_t current_angle = 0;
        servo_write_angle(current_angle, cfg);

        ESP_LOGI(TAG, "%s iniciado en GPIO %u", cfg->name, cfg->gpio);

        while (true)
        {
            ServoCmd cmd;

            if (xQueueReceive(g_queues.servo_cmd, &cmd, portMAX_DELAY) == pdTRUE)
            {
                ESP_LOGI(TAG,
                         "Comando recibido: target=%u tol=%u delay=%u step=%u",
                         cmd.target_angle,
                         cmd.tolerance_deg,
                         cmd.step_delay_ms,
                         cmd.step_deg);

                while (current_angle + cmd.tolerance_deg < cmd.target_angle ||
                       current_angle > cmd.target_angle + cmd.tolerance_deg)
                {
                    if (current_angle < cmd.target_angle)
                    {
                        uint16_t next = current_angle + cmd.step_deg;

                        if (next > cmd.target_angle)
                        {
                            next = cmd.target_angle;
                        }

                        current_angle = (uint8_t)next;
                    }
                    else if (current_angle > cmd.target_angle)
                    {
                        if (current_angle < cmd.step_deg)
                        {
                            current_angle = 0;
                        }
                        else
                        {
                            current_angle -= cmd.step_deg;
                        }

                        if (current_angle < cmd.target_angle)
                        {
                            current_angle = cmd.target_angle;
                        }
                    }

                    servo_write_angle(current_angle, cfg);

                    ServoStatusMsg moving = {};
                    moving.status = ServoStatusType::Moving;
                    moving.current_angle = current_angle;
                    moving.target_angle = cmd.target_angle;
                    moving.tick = xTaskGetTickCount();

                    xQueueOverwrite(g_queues.servo_status, &moving);
                    
                    ServoCmd new_cmd;

                    if (xQueueReceive(g_queues.servo_cmd, &new_cmd, 0) == pdTRUE)
                    {
                        cmd = new_cmd;

                        ESP_LOGI(TAG,
                                "Comando actualizado: target=%u delay=%u step=%u",
                                cmd.target_angle,
                                cmd.step_delay_ms,
                                cmd.step_deg);
                    }
                    
                    vTaskDelay(pdMS_TO_TICKS(cmd.step_delay_ms));
                }

                current_angle = cmd.target_angle;
                servo_write_angle(current_angle, cfg);

                ServoStatusMsg reached = {};
                reached.status = ServoStatusType::Reached;
                reached.current_angle = current_angle;
                reached.target_angle = cmd.target_angle;
                reached.tick = xTaskGetTickCount();

                xQueueOverwrite(g_queues.servo_status, &reached);

                ESP_LOGI(TAG, "Objetivo alcanzado: %u grados", current_angle);
            }
        }
    }
}