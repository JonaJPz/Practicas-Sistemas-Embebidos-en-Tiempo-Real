#include "task_manager.hpp"

extern "C" void app_main(void)
{
    App::app_tasks_create();
}

/*
 * CONCLUSION
 *
 * En esta practica se implemento un sistema embebido en tiempo real sobre
 * ESP32 utilizando FreeRTOS para coordinar multiples tareas mediante colas,
 * estructuras de configuracion y manejo de estados centralizado. Se comprobo
 * la utilidad de las colas para intercambiar informacion de manera segura
 * entre tareas, evitando los problemas asociados al uso de variables
 * globales compartidas.
 *
 * Asimismo, se aplico un filtro de mediana para mejorar la estabilidad de
 * las lecturas provenientes del sensor LDR, permitiendo tomar decisiones
 * mas confiables respecto al movimiento del servomotor. El uso de un
 * TaskManager como controlador principal permitio coordinar la suspension
 * y reanudacion de tareas mediante TaskHandle_t, garantizando que cada
 * componente operara unicamente cuando correspondia segun el estado del
 * sistema.
 *
 * Finalmente, la practica permitio reforzar conceptos fundamentales de
 * FreeRTOS como comunicacion entre tareas, sincronizacion, administracion
 * de estados y diseño modular de aplicaciones embebidas, mostrando como
 * estas herramientas facilitan el desarrollo de sistemas mas robustos,
 * escalables y faciles de mantener.
 */