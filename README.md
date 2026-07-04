# Implementación y Simulación del Sistema de Archivos FAT16 (C)

Este proyecto es una implementación a bajo nivel de las utilidades de gestión y lectura para el sistema de archivos FAT16, desarrollado en lenguaje C. Permite inspeccionar la estructura interna de volúmenes FAT16, navegar por sus directorios y manipular la tabla de asignación de archivos (FAT) interactuando directamente a nivel de bytes y clústeres.

## Características Técnicas
* **Lenguaje:** C estándar (C99/C11).
* **Manipulación Binaria:** Lectura directa del *Boot Sector* (Sector de Arranque) y mapeo de las estructuras de la FAT16 en memoria.
* **Gestión de Clústeres:** Algoritmos para el cálculo del tamaño de la tabla (16 bits por entrada), resolución del directorio raíz y seguimiento de las cadenas de clústeres para la lectura de archivos.
* **Herramientas Implementadas y Optimizadas:**
  * `fat16-info`: Lectura y extracción de metadatos del volumen.
  * `fat16-ls`: Listado del directorio raíz y subdirectorios.
  * `fat16-fileclusters`: Rastreo de los clústeres físicos asignados a un archivo específico.
  * `fat16-sh`: Shell interactiva con soporte para comandos de lectura (`cat`), listado (`ls`) y eliminación lógica a nivel de tabla (`rm`).

## Requisitos y Compilación

### Prerrequisitos
* Compilador GCC.
* Herramienta `make` instalada en el sistema (Linux/Unix/MinGW).

### Compilación
Para compilar y generar todos los ejecutables del sistema, ejecuta el siguiente comando en la raíz del proyecto:
```bash
make
```

### Ejecución de la Shell Interactiva
Una vez compilado, puedes montar la imagen de prueba e iniciar la shell:
```bash
./fat16-sh img/fat3
```

## Estructura del Proyecto

```text
simulador-fat16/
├── src/        # Código fuente de las utilidades y la shell (.c)
├── include/    # Archivos de cabecera (.h)
├── img/        # Imágenes de disco binarias de prueba
└── Makefile    # Reglas de compilación automatizada
```
