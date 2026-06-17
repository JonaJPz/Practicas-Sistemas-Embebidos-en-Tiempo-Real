## Respuestas a las preguntas

### 1. ¿Cuál es la diferencia entre usar una variable global y una cola para comunicar datos?
Una variable global permite que varias tareas accedan directamente a la misma información, pero puede generar condiciones de carrera si varias tareas leen o escriben simultáneamente. Una cola de FreeRTOS proporciona una comunicación segura y sincronizada entre tareas, permitiendo transferir datos sin necesidad de compartir memoria directamente.

### 2. ¿Qué tarea queda bloqueada cuando espera datos de una cola?
La tarea que ejecuta una función como `xQueueReceive()` con un tiempo de espera queda bloqueada automáticamente hasta que existan datos disponibles en la cola o expire el tiempo de espera configurado.

### 3. ¿Por qué TaskManager debe concentrar las decisiones del sistema?
Porque centraliza el control de estados y evita que múltiples tareas modifiquen el comportamiento global de forma independiente. Esto mejora la organización del código, reduce errores de sincronización y facilita el mantenimiento de la aplicación.

### 4. ¿Por qué pvParameters es más flexible que crear una función distinta por tarea?
Porque permite reutilizar la misma función de tarea con diferentes configuraciones. Cada tarea recibe una estructura de parámetros propia, evitando duplicar código y facilitando la escalabilidad del sistema.

### 5. ¿Qué diferencia existe entre suspender una tarea y bloquearla esperando una cola?
Una tarea suspendida no puede ejecutarse hasta que otra tarea la reanude explícitamente mediante `vTaskResume()`. En cambio, una tarea bloqueada esperando una cola se reactiva automáticamente cuando llegan datos o cuando vence el tiempo de espera configurado.

### 6. ¿Qué efecto tiene aumentar el tamaño de la ventana del filtro de mediana?
Un tamaño de ventana mayor reduce más eficazmente el ruido y los valores atípicos, produciendo lecturas más estables. Sin embargo, también aumenta el retardo de respuesta ante cambios rápidos en la señal del sensor.

### 7. ¿Por qué un filtro de mediana rechaza picos mejor que un promedio simple?
Porque selecciona el valor central de un conjunto ordenado de muestras en lugar de calcular una media. De esta forma, valores extremadamente altos o bajos tienen poca influencia sobre el resultado final.

### 8. ¿Qué ocurre si se presiona Start-operation mientras el servo está en movimiento?
No ocurre ninguna acción adicional, ya que la tarea asociada al botón Start-operation se encuentra suspendida durante la operación. Esto evita reinicios accidentales y garantiza el cumplimiento de la secuencia de funcionamiento.

### 9. ¿Cómo se garantiza que el botón de velocidad solo funcione durante la operación?
TaskManager mantiene suspendida la tarea del botón de velocidad cuando el sistema está en reposo y únicamente la reanuda al iniciar una operación. Al finalizar la operación, la tarea vuelve a suspenderse.

### 10. ¿Por qué un constexpr en C++ es preferible a #define para constantes tipadas?
Porque `constexpr` respeta el sistema de tipos de C++, permite validación durante la compilación, mejora la depuración y evita problemas asociados con la sustitución textual realizada por el preprocesador mediante `#define`.

## Conclusión

En esta práctica se implementó un sistema embebido en tiempo real sobre ESP32 utilizando FreeRTOS para coordinar múltiples tareas mediante colas, estructuras de configuración y manejo de estados centralizado. Se comprobó la utilidad de las colas para intercambiar información de manera segura entre tareas, evitando los problemas asociados al uso de variables globales compartidas.

Asimismo, se aplicó un filtro de mediana para mejorar la estabilidad de las lecturas provenientes del sensor LDR, permitiendo tomar decisiones más confiables respecto al movimiento del servomotor. El uso de un TaskManager como controlador principal permitió coordinar la suspensión y reanudación de tareas mediante `TaskHandle_t`, garantizando que cada componente operara únicamente cuando correspondía según el estado del sistema.

Finalmente, la práctica permitió reforzar conceptos fundamentales de FreeRTOS como comunicación entre tareas, sincronización, administración de estados y diseño modular de aplicaciones embebidas, mostrando cómo estas herramientas facilitan el desarrollo de sistemas más robustos, escalables y fáciles de mantener.