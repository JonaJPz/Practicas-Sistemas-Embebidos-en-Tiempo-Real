/**
 * @file    main.c
 * @brief   Punto de entrada del sistema de control de LEDs por potenciometros.
 *
 * =============================================================================
 * CONCLUSION DEL EQUIPO
 * Integrantes: Nombre Apellido, Nombre Apellido, Nombre Apellido
 *
 * El uso de pvParameters con una estructura task_params_t permite instanciar
 * una sola funcion de tarea (vTaskPotLED) tres veces con configuraciones
 * distintas, eliminando codigo duplicado y facilitando el mantenimiento.
 * Los bloques de parametros deben ser static (o en heap) porque las tareas
 * acceden a ellos durante toda su vida; declararlos locales en la funcion
 * que llama xTaskCreate() provocaria un uso de memoria invalida al regresar
 * del stack. Con las tres prioridades distintas (1, 2, 3) observamos que,
 * cuando varias tareas salen del estado bloqueado en el mismo tick, el
 * planificador preemptivo de FreeRTOS las ejecuta en orden descendente de
 * prioridad, lo que se refleja en el orden de los mensajes por UART.
 * =============================================================================
 *
 * Descripcion:
 *   Inicializa los perifericos (ADC y LEDC) y arranca tres tareas FreeRTOS.
 *   Cada tarea lee un potenciometro y ajusta el brillo del LED correspondiente.
 *
 *   La logica de negocio esta encapsulada en los modulos:
 *     - adc_reader : lectura de potenciometros via ADC oneshot
 *     - leds       : control de brillo via LEDC (PWM)
 *     - tasks      : tareas FreeRTOS y sus parametros
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x  / FreeRTOS
 * IDE        : VS Code + PlatformIO (framework: espidf)
 * Estandar   : C99 con restricciones MISRA-C:2012 aplicables
 */

#include "adc_reader.h"
#include "leds.h"
#include "tasks.h"

/* ------------------------------------------------------------------
 *  Punto de entrada
 * ------------------------------------------------------------------ */

void app_main(void)
{
    /* 1. Inicializar subsistema ADC (tres potenciometros) */
    adc_reader_init();

    /* 2. Inicializar subsistema LED PWM (tres canales LEDC) */
    leds_init();

    /* 3. Crear las tres tareas FreeRTOS; el scheduler ya esta activo */
    tasks_create_all();

    /* app_main retorna; FreeRTOS continua ejecutando las tareas */
}
