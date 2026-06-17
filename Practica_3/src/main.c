/**
 * @file    main.c
 * @brief   Punto de entrada del contador BCD con FreeRTOS.
 *
 * =============================================================================
 * CONCLUSION DEL EQUIPO
 * Integrantes: [Tu nombre] — IAMAA24-01, UNAQ
 *
 * Esta practica demuestra que vTaskSuspend/vTaskResume permite controlar
 * el ciclo de vida de una tarea sin destruirla: al pausar, la tarea pasa
 * al estado SUSPENDED conservando su stack y contexto, por lo que al
 * reanudar el contador continua exactamente desde el ultimo valor mostrado.
 * Esto es fundamentalmente distinto del estado BLOCKED (que usa vTaskDelay):
 * BLOCKED es temporal y gestionado por el planificador segun el tick del
 * sistema, mientras que SUSPENDED es indefinido y solo sale por
 * vTaskResume() explicitamente.
 *
 * El uso de pvParameters elimina la necesidad de crear tres funciones de
 * tarea distintas para los botones — una sola funcion vTaskButton recibe
 * el GPIO y el ID del boton, reduciendo el codigo a la mitad y facilitando
 * agregar un cuarto boton sin modificar ninguna firma existente.
 *
 * El Task Manager con eTaskGetState() permite introspeccionar el estado
 * real del planificador en tiempo de ejecucion, lo que es invaluable para
 * depuracion de sistemas multitarea embebidos.
 * =============================================================================
 *
 * Descripcion:
 *   Contador BCD ascendente/descendente de 0 a 9 mostrado en un display
 *   de 7 segmentos de catodo comun. Tres botones controlan el Start/Pause,
 *   la direccion de conteo y la velocidad (500 ms / 250 ms).
 *
 * Modulos:
 *   - system_state : estado compartido (mutex FreeRTOS)
 *   - leds         : driver del display 7 segmentos
 *   - buttons      : tareas de botones parametrizadas con pvParameters
 *   - counter      : tarea contadora BCD
 *   - app_tasks    : Task Manager y control de ciclo de vida
 *
 * Plataforma : ESP32 / ESP-IDF 5.x-6.x / FreeRTOS
 * IDE        : VS Code + PlatformIO (framework: espidf)
 */

#include "system_state.h"
#include "leds.h"
#include "buttons.h"
#include "app_tasks.h"

/* ------------------------------------------------------------------
 *  Punto de entrada
 * ------------------------------------------------------------------ */

void app_main(void)
{
    /* 1. Inicializar estado compartido del sistema (crea el mutex) */
    system_state_init();

    /* 2. Inicializar hardware: display y botones */
    leds_init();
    buttons_init();

    /* 3. Mostrar 0 en el display al arrancar */
    leds_show_digit(0U);

    /* 4. Crear todas las tareas FreeRTOS via Task Manager */
    app_tasks_create_all();

    /* app_main retorna; FreeRTOS ejecuta las tareas */
}
