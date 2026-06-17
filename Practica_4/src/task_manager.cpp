#include "task_manager.hpp"
#include "app_config.hpp"
#include "app_context.hpp"
#include "messages.hpp"
#include "sensor_task.hpp"
#include "button_task.hpp"
#include "servo_task.hpp"
#include "ready_led_task.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

namespace App
{
    static const char *TAG = "MANAGER";

    static SensorTaskConfig sensor_cfg
    {
        AppConfig::LDR_ADC_UNIT,
        AppConfig::LDR_ADC_CHANNEL,
        AppConfig::SENSOR_PERIOD_MS,
        AppConfig::FILTER_WINDOW_SIZE,
        "SensorTask"
    };

    static ServoTaskConfig servo_cfg
    {
        AppConfig::SERVO_GPIO,
        AppConfig::SERVO_PWM_CHANNEL,
        AppConfig::SERVO_PWM_TIMER,
        AppConfig::SERVO_PWM_MODE,
        AppConfig::SERVO_PWM_RES_BITS,
        AppConfig::SERVO_PWM_FREQ_HZ,
        AppConfig::SERVO_MIN_US,
        AppConfig::SERVO_MAX_US,
        "ServoTask"
    };

    static ButtonTaskConfig start_btn_cfg
    {
        AppConfig::START_BUTTON_GPIO,
        "StartButton",
        ButtonEventType::Start,
        AppConfig::BUTTON_POLL_MS,
        true
    };

    static ButtonTaskConfig speed_btn_cfg
    {
        AppConfig::SPEED_BUTTON_GPIO,
        "SpeedButton",
        ButtonEventType::SpeedState,
        AppConfig::BUTTON_POLL_MS,
        true
    };

    static ReadyLedTaskConfig ready_led_cfg
    {
        AppConfig::READY_LED_GPIO,
        AppConfig::READY_LED_ON_MS,
        AppConfig::READY_LED_OFF_MS,
        "ReadyLed"
    };

    static ManagerTaskConfig manager_cfg
    {
        AppConfig::HOLD_TARGET_MS,
        AppConfig::SERVO_TOLERANCE_DEG,
        AppConfig::SERVO_DELAY_SLOW_MS,
        AppConfig::SERVO_DELAY_FAST_MS,
        AppConfig::SERVO_STEP_DEG,
        "TaskManager"
    };

    enum class ManagerState
    {
        Ready,
        Operating,
        HoldingTarget
    };

    static const char *state_text(eTaskState st)
    {
        switch (st)
        {
            case eRunning: return "RUNNING";
            case eReady: return "READY";
            case eBlocked: return "BLOCKED";
            case eSuspended: return "SUSPENDED";
            case eDeleted: return "DELETED";
            default: return "UNKNOWN";
        }
    }

    static void send_servo_cmd(uint8_t target, bool fast, const ManagerTaskConfig *cfg)
    {
        ServoCmd cmd
        {
            target,
            cfg->tolerance_deg,
            fast ? cfg->fast_delay_ms : cfg->slow_delay_ms,
            cfg->step_deg
        };

        xQueueOverwrite(g_queues.servo_cmd, &cmd);
    }

    static void print_states()
    {
        ESP_LOGI(TAG,
                 "STATES sensor=%s servo=%s start=%s speed=%s ready=%s",
                 state_text(eTaskGetState(g_handles.sensor)),
                 state_text(eTaskGetState(g_handles.servo)),
                 state_text(eTaskGetState(g_handles.start_button)),
                 state_text(eTaskGetState(g_handles.speed_button)),
                 state_text(eTaskGetState(g_handles.ready_led)));
    }

    void TaskManager::run(void *pvParameters)
    {
        auto *cfg = static_cast<ManagerTaskConfig *>(pvParameters);

        bool fast_mode = false;
        bool target_reached = false;
        bool movement_started = false;

        uint8_t current_target = AppConfig::SERVO_ANGLE_DARK;

        TickType_t reached_tick = 0;
        TickType_t last_print_tick = 0;

        ManagerState state = ManagerState::Ready;

        vTaskSuspend(g_handles.sensor);
        vTaskSuspend(g_handles.servo);
        vTaskSuspend(g_handles.speed_button);
        vTaskResume(g_handles.start_button);
        vTaskResume(g_handles.ready_led);

        ESP_LOGI(TAG, "Task Manager iniciado");
        ESP_LOGI(TAG, "Estado inicial: READY");

        while (true)
        {
            ButtonMsg button_msg;

            while (xQueueReceive(g_queues.buttons, &button_msg, 0) == pdTRUE)
            {
                if (button_msg.type == ButtonEventType::Start && button_msg.pressed)
                {
                    if (state == ManagerState::Ready)
                    {
                        ESP_LOGI(TAG, "START presionado -> inicia operacion");

                        state = ManagerState::Operating;
                        fast_mode = false;
                        target_reached = false;
                        state = ManagerState::Operating;

                        vTaskSuspend(g_handles.ready_led);
                        vTaskSuspend(g_handles.start_button);

                        vTaskResume(g_handles.sensor);
                        vTaskResume(g_handles.servo);
                        vTaskResume(g_handles.speed_button);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "START ignorado: sistema ocupado");
                    }
                }

                if (button_msg.type == ButtonEventType::SpeedState)
                {
                    if (state == ManagerState::Operating)
                    {
                        fast_mode = button_msg.pressed;

                        ESP_LOGI(TAG,
                                 "SPEED %s -> modo %s",
                                 button_msg.pressed ? "PRESIONADO" : "LIBERADO",
                                 fast_mode ? "RAPIDO" : "LENTO");

                        send_servo_cmd(current_target, fast_mode, cfg);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "SPEED ignorado: sistema no operativo");
                    }
                }
            }

            if (state == ManagerState::Operating)
            {
                SensorMsg sensor_msg;

                while (xQueueReceive(g_queues.sensor, &sensor_msg, 0) == pdTRUE)
                {
                    ESP_LOGI(TAG,
                            "Sensor raw=%u filtered=%u target=%u",
                            sensor_msg.raw,
                            sensor_msg.filtered,
                            sensor_msg.target_angle);

                    if (!movement_started)
                    {
                        current_target = sensor_msg.target_angle;
                        movement_started = true;

                        ESP_LOGI(TAG,
                                "Target fijado para esta operacion: %u",
                                current_target);

                        send_servo_cmd(current_target, fast_mode, cfg);
                    }
                    else
                    {
                        ESP_LOGI(TAG,
                                "Cambio de luz ignorado durante movimiento. Target fijo=%u",
                                current_target);
                    }
                }

                ServoStatusMsg status_msg;

                while (xQueueReceive(g_queues.servo_status, &status_msg, 0) == pdTRUE)
                {
                    if (status_msg.status == ServoStatusType::Moving)
                    {
                        ESP_LOGI(TAG,
                                 "Servo moviendo actual=%u target=%u",
                                 status_msg.current_angle,
                                 status_msg.target_angle);
                    }

                    if (status_msg.status == ServoStatusType::Reached)
                        {
                            ESP_LOGI(TAG,
                                    "Servo alcanzo target=%u -> HOLD 8s",
                                    status_msg.target_angle);

                            target_reached = true;
                            reached_tick = xTaskGetTickCount();

                            vTaskSuspend(g_handles.sensor);
                            vTaskSuspend(g_handles.speed_button);
                            vTaskSuspend(g_handles.servo);
                            vTaskSuspend(g_handles.ready_led);
                            vTaskSuspend(g_handles.start_button);

                            state = ManagerState::HoldingTarget;
                        }
                }
            }

            if (state == ManagerState::HoldingTarget)
            {
                TickType_t elapsed_ticks = xTaskGetTickCount() - reached_tick;
                uint32_t elapsed_ms = elapsed_ticks * portTICK_PERIOD_MS;

                if (elapsed_ms >= cfg->hold_target_ms)
                {
                    ESP_LOGI(TAG, "HOLD terminado -> READY");

                    vTaskSuspend(g_handles.sensor);
                    vTaskSuspend(g_handles.servo);
                    vTaskSuspend(g_handles.speed_button);

                    vTaskResume(g_handles.start_button);
                    vTaskResume(g_handles.ready_led);

                    movement_started = false;
                    target_reached = false;

                    state = ManagerState::Ready;
                }
            }

            TickType_t now = xTaskGetTickCount();

            if ((now - last_print_tick) >= pdMS_TO_TICKS(3000))
            {
                last_print_tick = now;
                print_states();
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    void app_tasks_create()
    {
        g_queues.sensor = xQueueCreate(1, sizeof(SensorMsg));
        g_queues.buttons = xQueueCreate(AppConfig::BUTTON_QUEUE_LEN, sizeof(ButtonMsg));
        g_queues.servo_cmd = xQueueCreate(1, sizeof(ServoCmd));
        g_queues.servo_status = xQueueCreate(1, sizeof(ServoStatusMsg));

        xTaskCreate(SensorTask::run,
                    sensor_cfg.name,
                    4096,
                    &sensor_cfg,
                    3,
                    &g_handles.sensor);

        xTaskCreate(ServoTask::run,
                    servo_cfg.name,
                    4096,
                    &servo_cfg,
                    3,
                    &g_handles.servo);

        xTaskCreate(ButtonTask::run,
                    start_btn_cfg.name,
                    2048,
                    &start_btn_cfg,
                    4,
                    &g_handles.start_button);

        xTaskCreate(ButtonTask::run,
                    speed_btn_cfg.name,
                    2048,
                    &speed_btn_cfg,
                    4,
                    &g_handles.speed_button);

        xTaskCreate(ReadyLedTask::run,
                    ready_led_cfg.name,
                    2048,
                    &ready_led_cfg,
                    1,
                    &g_handles.ready_led);

        xTaskCreate(TaskManager::run,
                    manager_cfg.name,
                    4096,
                    &manager_cfg,
                    5,
                    &g_handles.manager);
    }
}