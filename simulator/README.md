# Simulador de Vuelo en C++ y OpenGL

Un simulador de vuelo completo, con 6 grados de libertad (6-DOF), desarrollado en C++17 y OpenGL 4.6. Este proyecto combina una simulación de física de cuerpo rígido rigurosa con un pipeline de renderizado 3D dinámico y generado proceduralmente.

## Características Principales

* **Modelo de Dinámica de Vuelo Avanzado (FDM):** Implementa una simulación aerodinámica de 6-DOF basada en los modelos de vuelo atmosférico de Stevens & Lewis. Calcula en tiempo real la sustentación, la resistencia y los momentos utilizando coeficientes aerodinámicos no dimensionales, traduciendo las fuerzas del marco de viento a cinemática precisa del marco del cuerpo mediante integración de Euler.
* **Terreno Infinito Procedural:** Utiliza ruido de movimiento browniano fractal (fBm) para generar terreno dinámico basado en fragmentos (chunks). El motor transmite fragmentos dentro y fuera de la memoria según la proximidad de la cámara y realiza detección de colisiones en tiempo real mediante muestreo de altura procedimental.
* **Sistema de Aviónica Personalizado (HUD):** Cuenta con un gestor de renderizado de gráficos vectoriales 2D personalizado que dibuja instrumentos de vuelo críticos en tiempo real, incluyendo una escalera de cabeceo (horizonte artificial), cintas de velocidad y altitud, un variómetro y una brújula dinámica.
* **Shadow Mapping:** Implementa un pipeline de renderizado de dos pasadas utilizando objetos de framebuffer (FBO) para proyectar sombras dinámicas de alta resolución desde la geometría del avión y el terreno basándose en el vector direccional del sol.
* **Navegación consciente del terreno:** Incluye un sistema de waypoints dinámico que genera objetivos de navegación programáticamente, consultando el terreno procedimental para garantizar espacios seguros.

---

## Arquitectura del Proyecto

El repositorio está modularizado en dominios distintos, separando el motor de renderizado de la física subyacente y las dependencias externas.

```text
├── CGyAV-dlfdm/          # Motor de dinámica de vuelo y aerodinámica
│   ├── include/dlfdm/    # Cabeceras de física (dinámica de aeronaves, modelos aerodinámicos)
│   └── src/dlfdm/        # Implementaciones de física
├── glad/                 # Configuración del cargador OpenGL
├── simulator/            # Aplicación principal y motor de renderizado
│   ├── airplane/         # Carga de modelos 3D, transformaciones y shaders del avión
│   ├── camera/           # Matemáticas de la matriz de vista (modos Chase, Front, First-Person)
│   ├── helper/           # Traducciones de coordenadas (NED a espacio de mundo OpenGL)
│   ├── hud/              # Gestor de UI vectorial 2D e instrumentos de aviónica
│   ├── main/             # Punto de entrada de la aplicación y bucle de juego de paso fijo
│   ├── map/              # Generación de terreno procedimental, ruido fBm y skybox
│   ├── opengl/           # Contexto de ventana GLFW y gestión de entradas
│   ├── textures/         # Activos (cubemaps de skybox, terreno y texturas del avión)
│   └── waypoint/         # Lógica de navegación y generación consciente del terreno
├── stb/                  # Biblioteca de análisis de imágenes (stb_image)
├── stb_impl.cpp          # Archivo de implementación para stb_image
└── Makefile              # Configuración de compilación e instrucciones del enlazador

```

---

## Controles

El simulador utiliza una disposición de teclado estándar para manipular las superficies de control y el acelerador de la aeronave.

* **W / S:** Cabeceo (Elevador)
* **A / D:** Alabeo (Alerón)
* **Q / E:** Guiñada (Timón)
* **Mayús Izq.:** Aumentar acelerador
* **Ctrl Izq.:** Disminuir acelerador
* **C:** Cambiar modos de cámara (Back -> Front -> Cabina)
* **Escape:** Terminar aplicación

---

## Instrucciones de Compilación

Este proyecto está configurado para compilarse en Windows utilizando MSYS2 / MinGW64. Requiere un compilador compatible con C++17 y enlaces contra GLFW3 y OpenGL32.

### Requisitos previos

Asegúrate de tener instalada la cadena de herramientas MinGW64 y correctamente añadida a la ruta de tu sistema (PATH). Necesitarás las siguientes bibliotecas de desarrollo:

* `mingw-w64-x86_64-gcc`
* `mingw-w64-x86_64-glfw`
* `make`

### Compilación

Navega al directorio raíz del proyecto donde se encuentra el `Makefile` y ejecuta el comando de compilación:

```bash
make

```

El Makefile compila todos los archivos fuente con la bandera de optimización `-O2` y enlaza las dependencias necesarias (`-lglfw3`, `-lopengl32`, `-lgdi32`).

### Ejecución

Una vez completada la compilación, puedes iniciar el simulador directamente desde la terminal:

```bash
make run

```

Para limpiar el ejecutable compilado y forzar una compilación limpia en la próxima ejecución:

```bash
make clean

```