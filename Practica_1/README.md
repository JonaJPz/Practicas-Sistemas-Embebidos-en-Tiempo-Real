# Práctica 1 – Tareas, Prioridades, Idle Task y Ciclo LED/Sensor

## Integrantes

- Jonathan Josué Juárez Pérez
- Rodrigo Alberto Romero Beltran 

---

## Descripción del proyecto

En esta práctica se implementó un sistema multitarea utilizando FreeRTOS sobre una tarjeta ESP32 mediante PlatformIO. El sistema controla el parpadeo de un LED en dos modos de funcionamiento: rápido y lento, los cuales son seleccionados mediante un pulsador.

Además, se implementó una tarea encargada de leer un potenciómetro conectado a una entrada ADC y reportar los valores obtenidos a través del puerto serial UART. También se incorporó una tarea de monitoreo para detectar eventos del botón y supervisar recursos del sistema, como memoria libre y uso de pila.

La práctica permitió aplicar conceptos fundamentales de FreeRTOS como creación de tareas, prioridades, suspensión y reanudación de tareas, comunicación mediante variables compartidas e implementación del Idle Hook.

---

## Hardware utilizado

- ESP32 DevKit V1
- LED integrado (GPIO2)
- Pulsador BOOT (GPIO0)
- Potenciómetro de 10 kΩ
- Cable USB para programación y monitoreo serial

---

## Software utilizado

- Visual Studio Code
- PlatformIO
---



# Preguntas de análisis

## 1. ¿Por qué la variable `g_ledRapido` debe declararse como `volatile`? ¿Qué ocurre si se omite esa palabra clave?

La variable `g_ledRapido` es compartida entre varias tareas de FreeRTOS. Al declararla como `volatile`, se le indica al compilador que su valor puede cambiar en cualquier momento fuera del flujo normal de ejecución de una tarea.

Si se omite esta palabra clave, el compilador podría optimizar el código almacenando el valor en un registro y reutilizándolo sin volver a leer la memoria. Como consecuencia, una tarea podría no detectar los cambios realizados por otra tarea, provocando comportamientos incorrectos en la lógica del sistema.

---

## 2. ¿En qué momento exacto aparece el mensaje `[IDLE]` en la terminal? Describe el estado de las cuatro tareas en ese instante.

El mensaje `[IDLE]` aparece cuando ninguna de las tareas de usuario se encuentra en estado **Ready** o **Running**. Esto significa que todas las tareas están bloqueadas, normalmente debido a llamadas a `vTaskDelay()`.

En ese instante:

- `vTaskLedRapido` está suspendida o bloqueada.
- `vTaskLedLento` está esperando su siguiente periodo de ejecución.
- `vTaskSensor` está bloqueada durante su tiempo de muestreo.
- `vTaskMonitor` está esperando su siguiente ciclo de monitoreo.

Al no existir tareas listas para ejecutarse, el planificador ejecuta automáticamente la **Idle Task** y se llama a `vApplicationIdleHook()`.

---

## 3. ¿Qué diferencia existe entre `vTaskDelay()` y `vTaskDelayUntil()`? ¿En cuál de las tareas de esta práctica sería más apropiado usar `vTaskDelayUntil()`?

`vTaskDelay()` bloquea una tarea durante una cantidad específica de ticks contados desde el momento en que se realiza la llamada.

Por otro lado, `vTaskDelayUntil()` permite ejecutar tareas de forma periódica manteniendo un periodo fijo respecto a una referencia temporal inicial. Esto evita que pequeñas variaciones en el tiempo de ejecución acumulen error.

En esta práctica sería más apropiado utilizar `vTaskDelayUntil()` en la tarea `vTaskSensor`, ya que la adquisición de datos del ADC debe realizarse cada 300 ms de manera constante y precisa.

---

## 4. ¿Por qué `vTaskLedRapido` tiene prioridad menor que `vTaskMonitor`? Describe qué ocurriría si se invirtieran esas prioridades.

La tarea `vTaskMonitor` tiene una prioridad mayor porque es responsable de detectar la pulsación del botón y supervisar el estado general del sistema.

Si `vTaskLedRapido` tuviera una prioridad superior, podría ejecutarse con mayor frecuencia que la tarea de monitoreo, retrasando la detección de eventos del botón y reduciendo la capacidad de respuesta del sistema. Aunque el sistema seguiría funcionando, la transición entre modos podría presentar retardos perceptibles.

---

## 5. ¿Qué riesgo existe al leer una variable `volatile` desde dos tareas distintas sin protección? Investiga el concepto de sección crítica.

La palabra clave `volatile` únicamente garantiza que el compilador lea siempre el valor actualizado desde memoria, pero no protege el acceso concurrente.

Si dos tareas acceden simultáneamente a una misma variable compartida y al menos una de ellas la modifica, pueden producirse condiciones de carrera (*race conditions*), obteniendo datos inconsistentes o inesperados.

Una sección crítica es una región de código donde se protege el acceso a recursos compartidos para evitar interrupciones o accesos simultáneos. En FreeRTOS se pueden utilizar mecanismos como:

```cpp
taskENTER_CRITICAL();

/* Código crítico */

taskEXIT_CRITICAL();
```

para asegurar que una operación sobre una variable compartida se realice de forma atómica.

---

# Conclusiones

Durante esta práctica se implementó un sistema multitarea basado en FreeRTOS utilizando una ESP32. Se comprobó el funcionamiento del planificador mediante la ejecución concurrente de tareas con diferentes prioridades, así como el uso de variables compartidas para coordinar el comportamiento del sistema.

También se observó la importancia de los mecanismos de temporización, suspensión y reanudación de tareas para implementar máquinas de estados simples. La utilización del ADC permitió integrar la lectura de sensores dentro de la arquitectura multitarea, mientras que la supervisión mediante UART facilitó la observación del comportamiento interno del sistema.

Finalmente, la implementación del Idle Hook permitió identificar los periodos en los que el procesador no tenía tareas listas para ejecutarse, demostrando el funcionamiento del planificador de FreeRTOS y la administración eficiente de los recursos de procesamiento. Esta práctica fortaleció los conocimientos sobre sistemas embebidos en tiempo real y la organización de aplicaciones multitarea en ESP32.
