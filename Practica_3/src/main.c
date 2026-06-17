/**
 * @file    main.c
 * @brief   Punto de entrada del contador BCD con FreeRTOS.
 * =============================================================================
 * CONCLUSION DE LA PRACTICA
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
 * /*
 * PREGUNTAS GUIA - PRACTICA 3
 *
 * 1. ¿Que diferencia existe entre BLOCKED y SUSPENDED?
 *
 * Una tarea en estado BLOCKED esta esperando que ocurra un
 * evento o que transcurra un tiempo determinado para volver
 * automaticamente al estado READY. Un ejemplo es cuando se
 * utiliza vTaskDelay().
 *
 * Una tarea en estado SUSPENDED ha sido detenida mediante
 * vTaskSuspend() y no volvera a ejecutarse hasta que otra
 * tarea invoque vTaskResume(). La reactivacion no ocurre de
 * forma automatica.
 *
 * ----------------------------------------------------------
 *
 * 2. ¿Por que vTaskDelay() coloca una tarea en estado BLOCKED?
 *
 * Porque la tarea debe esperar una cantidad especifica de
 * ticks antes de continuar su ejecucion. Durante ese tiempo
 * el planificador libera el procesador para que otras tareas
 * puedan ejecutarse. Una vez transcurrido el tiempo indicado,
 * la tarea vuelve automaticamente al estado READY.
 *
 * ----------------------------------------------------------
 *
 * 3. ¿Que diferencia existe entre vTaskDelay() y un Software Timer?
 *
 * vTaskDelay() bloquea una tarea completa durante un tiempo
 * determinado, mientras que un Software Timer ejecuta una
 * funcion callback cuando expira un temporizador.
 *
 * Los Software Timers son utiles para acciones periodicas
 * simples sin necesidad de crear una tarea adicional,
 * mientras que vTaskDelay() se utiliza para controlar la
 * periodicidad de ejecucion de una tarea.
 *
 * ----------------------------------------------------------
 *
 * 4. ¿Que funcion cumple el Idle Task?
 *
 * El Idle Task es una tarea creada automaticamente por
 * FreeRTOS con la prioridad mas baja. Se ejecuta unicamente
 * cuando ninguna otra tarea se encuentra en estado READY.
 *
 * Ademas de mantener ocupado al procesador cuando no existen
 * otras tareas disponibles, tambien realiza funciones
 * internas del sistema como la liberacion de recursos de
 * tareas eliminadas.
 *
 * ----------------------------------------------------------
 *
 * 5. ¿Como decide FreeRTOS cual tarea ejecutar cuando varias
 * tienen la misma prioridad?
 *
 * FreeRTOS utiliza el algoritmo Round Robin cuando varias
 * tareas de igual prioridad estan en estado READY. Cada tarea
 * recibe una porcion de tiempo de CPU (time slice) y al
 * terminar dicho intervalo el procesador es asignado a la
 * siguiente tarea de la misma prioridad.
 *
 * Esto permite repartir equitativamente el tiempo de
 * ejecucion entre todas las tareas que comparten prioridad.
 *
 * ----------------------------------------------------------
 *
 * 6. ¿Que ventajas aporta pvParameters?
 *
 * pvParameters permite enviar datos a una tarea durante su
 * creacion mediante xTaskCreate().
 *
 * Entre sus ventajas destacan la reutilizacion de codigo,
 * la reduccion del uso de variables globales, una mayor
 * modularidad y la posibilidad de configurar distintas
 * tareas utilizando una misma funcion.
 *
 * En esta practica se utilizo para parametrizar tareas de
 * LEDs y botones.
 *
 * ----------------------------------------------------------
 *
 * 7. ¿Que ventajas aporta TaskHandle_t?
 *
 * TaskHandle_t es un identificador que permite controlar una
 * tarea despues de haber sido creada.
 *
 * Gracias a el es posible suspender tareas, reanudarlas,
 * consultar sus estados, eliminarlas e implementar un
 * Task Manager centralizado para administrar el sistema.
 *
 * ----------------------------------------------------------
 *
 * 8. ¿Que ocurriria si el contador se implementara con
 * variables globales unicamente?
 *
 * Aunque el sistema podria funcionar, aumentaria el
 * acoplamiento entre modulos y seria mas dificil mantener
 * el codigo.
 *
 * Ademas podrian aparecer condiciones de carrera cuando
 * varias tareas accedan simultaneamente a las mismas
 * variables. Tambien se perderian las ventajas de
 * modularidad, escalabilidad y control que ofrece FreeRTOS
 * mediante el uso de tareas, pvParameters y TaskHandle_t.
 *
 *------------------------------------------------------------
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
