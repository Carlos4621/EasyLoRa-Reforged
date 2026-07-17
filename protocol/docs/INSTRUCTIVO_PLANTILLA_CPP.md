# Instructivo de uso de la plantilla de biblioteca C++

Esta plantilla crea una **biblioteca C++23** preparada para trabajar con:

- CMake 3.28 o posterior.
- Ninja.
- vcpkg en modo manifiesto.
- Clang, GCC y MSVC.
- GoogleTest.
- AddressSanitizer y UndefinedBehaviorSanitizer.
- ThreadSanitizer.
- clang-tidy y cppcheck.
- clang-format.
- Cobertura con GCC y gcovr.
- Instalación y consumo mediante `find_package()`.

La biblioteca se genera como **estática por defecto**, pero también puede compilarse como biblioteca compartida.

---

# 1. Requisitos

## Ubuntu y distribuciones basadas en Debian

Instala las herramientas principales:

```bash
sudo apt update

sudo apt install \
    git \
    cmake \
    ninja-build \
    clang \
    clang-tidy \
    clang-format \
    gcc \
    g++ \
    cppcheck \
    gcovr
```

Comprueba que estén disponibles:

```bash
git --version
cmake --version
ninja --version
clang++ --version
g++ --version
clang-tidy --version
clang-format --version
```

La plantilla requiere:

```text
CMake 3.28 o posterior
```

`ctest` se instala junto con CMake.

## Windows

Para el preset de MSVC se necesita:

- Visual Studio 2022.
- El componente **Desarrollo para el escritorio con C++**.
- CMake.
- Git.

Los scripts `.sh` están pensados principalmente para Linux, WSL o Git Bash. En PowerShell pueden ejecutarse directamente los comandos de CMake indicados en este instructivo.

---

# 2. Crear un proyecto

Extrae la plantilla y entra en su directorio:

```bash
unzip cpp_library_template.zip
cd cpp_library_template
```

Genera un proyecto:

```bash
bash ./script.sh NombreDelProyecto /ruta/destino
```

Ejemplo:

```bash
bash ./script.sh MotorControl "$HOME/Proyectos"
```

El proyecto quedará en:

```text
$HOME/Proyectos/MotorControl
```

El script realiza automáticamente:

1. Copia la estructura de la plantilla.
2. Sustituye los nombres internos.
3. Prepara una copia local de vcpkg en `.tools/vcpkg`.
4. Configura CMake mediante el preset `clang-debug`.

El script **no**:

- Inicializa un repositorio Git.
- Compila la biblioteca.
- Ejecuta las pruebas.

Al terminar:

```bash
cd "$HOME/Proyectos/MotorControl"
```

## Crear solamente los archivos

Para evitar la descarga de vcpkg y la configuración inicial:

```bash
bash ./script.sh MotorControl "$HOME/Proyectos" --no-configure
```

Después puede prepararse manualmente:

```bash
cd "$HOME/Proyectos/MotorControl"

bash scripts/bootstrap_vcpkg.sh
cmake --preset clang-debug
```

## Elegir otro preset inicial

```bash
bash ./script.sh MotorControl "$HOME/Proyectos" --preset gcc-debug
```

También puede usarse una variable de entorno:

```bash
CPP_TEMPLATE_PRESET=gcc-debug \
    bash ./script.sh MotorControl "$HOME/Proyectos"
```

---

# 3. Transformación del nombre

El nombre visible se convierte internamente a `snake_case`.

Por ejemplo:

```bash
bash ./script.sh MotorControl "$HOME/Proyectos"
```

produce:

| Elemento | Resultado |
|---|---|
| Directorio | `MotorControl` |
| Proyecto CMake | `motor_control` |
| Target de CMake | `motor_control` |
| Namespace de C++ | `motor_control` |
| Nombre de vcpkg | `motor-control` |
| Header principal | `motor_control.hpp` |
| Macro de exportación | `MOTOR_CONTROL_EXPORT` |

La biblioteca se enlaza desde CMake con:

```cmake
target_link_libraries(mi_aplicacion
    PRIVATE
        motor_control::motor_control
)
```

La API se incluye con:

```cpp
#include <motor_control/motor_control.hpp>
```

## Nombres que deben evitarse

No utilices nombres reservados por CMake, CTest o el backend de compilación, como:

```text
test
install
clean
all
help
package
edit_cache
rebuild_cache
```

Por ejemplo, un proyecto llamado `test` intentaría crear un target llamado `test`, que entra en conflicto con el target reservado de CTest.

Utiliza nombres descriptivos:

```text
RobotCore
SerialProtocol
ImageProcessing
NetworkTransport
MathUtilities
```

---

# 4. Estructura del proyecto generado

Ejemplo para `MotorControl`:

```text
MotorControl/
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── README.md
├── .clang-format
├── .clang-tidy
├── .editorconfig
├── .gitignore
├── include/
│   └── motor_control/
│       └── motor_control.hpp
├── src/
│   └── motor_control.cpp
├── tests/
│   ├── CMakeLists.txt
│   └── test_motor_control.cpp
├── cmake/
│   ├── CompilerWarnings.cmake
│   ├── Coverage.cmake
│   ├── ProjectConfig.cmake.in
│   ├── ProjectOptions.cmake
│   ├── Sanitizers.cmake
│   └── StaticAnalyzers.cmake
├── scripts/
│   ├── bootstrap_vcpkg.sh
│   ├── configure.sh
│   ├── build.sh
│   ├── test.sh
│   ├── analyze.sh
│   ├── format.sh
│   └── install_local.sh
├── docs/
│   ├── guia_entorno_cpp_vcpkg.md
│   └── INSTRUCTIVO_USO.md
└── .github/
    └── workflows/
        └── ci.yml
```

Durante la configuración se generan:

```text
.tools/vcpkg/
build/
```

Durante la instalación local se genera:

```text
out/
```

Estos directorios no deben editarse manualmente.

---

# 5. Organización de la biblioteca

## API pública

Los headers que utilizarán otros proyectos deben colocarse en:

```text
include/nombre_del_proyecto/
```

Ejemplo:

```text
include/motor_control/controller.hpp
```

Uso desde un consumidor:

```cpp
#include <motor_control/controller.hpp>
```

## Implementación privada

Los `.cpp` y headers internos deben colocarse en:

```text
src/
```

Ejemplo:

```text
src/controller.cpp
src/detail/parser.hpp
src/detail/parser.cpp
```

Los archivos de `src/` no forman parte de la API pública ni se instalan.

## Ejemplo de clase pública

Header:

```cpp
#pragma once

#include <motor_control/motor_control_export.hpp>

namespace motor_control {

class MOTOR_CONTROL_EXPORT Controller
{
public:
    Controller();

    void start();
    void stop();

    [[nodiscard]] bool is_running() const noexcept;

private:
    bool running_{false};
};

} // namespace motor_control
```

Implementación:

```cpp
#include <motor_control/controller.hpp>

namespace motor_control {

Controller::Controller() = default;

void Controller::start()
{
    running_ = true;
}

void Controller::stop()
{
    running_ = false;
}

bool Controller::is_running() const noexcept
{
    return running_;
}

} // namespace motor_control
```

La macro `MOTOR_CONTROL_EXPORT` es necesaria para exportar correctamente la API cuando la biblioteca se compila como compartida, especialmente en Windows.

---

# 6. Añadir archivos al proyecto

Cuando se agreguen nuevos `.cpp`, deben declararse en `CMakeLists.txt`.

Configuración inicial:

```cmake
add_library(${PROJECT_NAME}
    src/motor_control.cpp
)
```

Después de añadir archivos:

```cmake
add_library(${PROJECT_NAME}
    src/motor_control.cpp
    src/controller.cpp
    src/detail/parser.cpp
)
```

También pueden enumerarse headers públicos para facilitar su visualización en algunos IDEs:

```cmake
add_library(${PROJECT_NAME}
    src/motor_control.cpp
    src/controller.cpp

    include/motor_control/motor_control.hpp
    include/motor_control/controller.hpp
)
```

Se recomienda mantener una lista explícita y evitar usar `file(GLOB ...)` como mecanismo principal para descubrir fuentes.

---

# 7. Flujo normal de desarrollo

Este es el flujo más frecuente.

## Primera configuración

El generador ya ejecuta normalmente la configuración mediante:

```bash
bash scripts/configure.sh clang-debug
```

Si el proyecto se creó con `--no-configure`, ejecuta el mismo comando. El
script prepara vcpkg automáticamente cuando sea necesario:

```bash
bash scripts/configure.sh clang-debug
```

## Compilar

```bash
cmake --build --preset clang-debug
```

O mediante el script abreviado:

```bash
bash scripts/build.sh clang-debug
```

## Ejecutar pruebas

```bash
ctest --preset clang-debug
```

O:

```bash
bash scripts/test.sh clang-debug
```

## Flujo completo

```bash
bash scripts/configure.sh clang-debug
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

`configure.sh` utiliza:

```bash
cmake --preset clang-debug --fresh
```

`--fresh` limpia la caché de configuración de ese preset y vuelve a resolver las dependencias y opciones. No elimina el código fuente.

## clangd y el encabezado de exportación generado

CMake crea el encabezado público de exportación dentro de:

```text
build/<preset>/generated/<biblioteca>/<biblioteca>_export.hpp
```

CMake genera `compile_commands.json` directamente dentro del directorio del
preset. El archivo `.clangd` incluido en la raíz contiene:

```yaml
CompileFlags:
  CompilationDatabase: build/clang-debug
```

Por tanto, clangd lee directamente:

```text
build/clang-debug/compile_commands.json
```

No se crea ninguna copia ni enlace simbólico. `clang-debug` funciona como la
configuración de desarrollo para el editor y contiene las rutas de `include/` y
`build/clang-debug/generated/`.

Configura ese preset al menos una vez:

```bash
bash scripts/configure.sh clang-debug
```

Puedes compilar y probar después con `clang-asan`, `clang-tsan`, `release` u
otros presets sin cambiar la base que utiliza el editor. Si clangd conserva
diagnósticos anteriores tras configurar el proyecto, reinicia su servidor desde
el editor.

## Ciclo cotidiano recomendado

Cuando solo se modifican `.cpp` o headers:

```bash
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

Cuando se modifica `CMakeLists.txt`, `vcpkg.json`, presets o dependencias:

```bash
bash scripts/configure.sh clang-debug
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

---

# 8. Presets incluidos

Cada preset utiliza un directorio independiente:

```text
build/clang-debug/
build/clang-asan/
build/clang-tsan/
build/clang-tidy/
build/gcc-debug/
build/gcc-coverage/
build/release/
build/shared-release/
build/msvc-debug/
```

Nunca reutilices el mismo directorio de compilación para compiladores o sanitizers distintos.

---

# 9. Clang Debug

Es la configuración principal para desarrollar.

```bash
bash scripts/configure.sh clang-debug
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

Equivalente:

```bash
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug
```

Características:

- Clang.
- C++23.
- `Debug`.
- Biblioteca estática.
- Pruebas habilitadas.
- GoogleTest proporcionado por vcpkg.
- Advertencias estrictas.
- Símbolos de depuración.

---

# 10. ASan y UBSan

El preset `clang-asan` activa simultáneamente:

- AddressSanitizer.
- UndefinedBehaviorSanitizer.

## Qué puede detectar ASan

- Accesos fuera de los límites de arrays.
- Lecturas o escrituras fuera del heap.
- `use-after-free`.
- `double-free`.
- Liberaciones inválidas.
- Algunos errores de stack.
- Fugas de memoria en plataformas compatibles.

## Qué puede detectar UBSan

- Overflow de enteros con signo.
- Shifts inválidos.
- Desreferencia de punteros nulos.
- Accesos desalineados.
- Algunos casts inválidos.
- Estados inválidos de objetos.
- Otros casos de comportamiento indefinido.

## Flujo

```bash
bash scripts/configure.sh clang-asan
bash scripts/build.sh clang-asan
bash scripts/test.sh clang-asan
```

Equivalente:

```bash
cmake --preset clang-asan
cmake --build --preset clang-asan
ctest --preset clang-asan
```

El preset de pruebas configura:

```text
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1
UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
```

Esto hace que las pruebas se detengan ante el primer error y que UBSan intente mostrar un stack trace.

## Cuándo ejecutarlo

Ejecuta ASan/UBSan:

- Antes de integrar cambios importantes.
- Después de modificar manejo de memoria.
- Después de trabajar con punteros, buffers o arrays.
- Después de cambiar ownership.
- Antes de publicar una versión.
- De manera periódica aunque las pruebas normales pasen.

## Importante

Los sanitizers solo detectan errores en rutas que realmente se ejecutan. Una buena suite de pruebas aumenta su utilidad.

---

# 11. ThreadSanitizer

El preset `clang-tsan` activa ThreadSanitizer.

Puede detectar:

- Data races.
- Accesos concurrentes sin sincronización.
- Algunos usos incorrectos de mutexes.
- Errores de sincronización entre threads.

## Flujo

```bash
bash scripts/configure.sh clang-tsan
bash scripts/build.sh clang-tsan
bash scripts/test.sh clang-tsan
```

Equivalente:

```bash
cmake --preset clang-tsan
cmake --build --preset clang-tsan
ctest --preset clang-tsan
```

## Cuándo ejecutarlo

Úsalo cuando el proyecto contenga:

- `std::thread`.
- `std::jthread`.
- Boost.Asio con varios workers.
- Callbacks concurrentes.
- Variables compartidas.
- Mutexes.
- Atomics.
- Colas concurrentes.
- Pools de threads.

## ASan y TSan

ASan y TSan deben utilizarse en compilaciones separadas. No intentes activarlos simultáneamente.

---

# 12. clang-tidy

El preset `clang-tidy` activa el análisis estático durante la compilación.

Flujo recomendado:

```bash
bash scripts/analyze.sh
```

Internamente ejecuta:

```bash
bash scripts/configure.sh clang-tidy
cmake --build --preset clang-tidy --parallel
```

También puede ejecutarse manualmente:

```bash
cmake --preset clang-tidy
cmake --build --preset clang-tidy
```

La configuración está en:

```text
.clang-tidy
```

Incluye familias como:

```text
bugprone-*
clang-analyzer-*
cppcoreguidelines-*
modernize-*
performance-*
portability-*
readability-*
```

No todas las advertencias representan errores obligatorios. Revisa cada diagnóstico antes de modificar el diseño.

---

# 13. cppcheck

cppcheck está soportado, pero no tiene un preset dedicado.

Para un proyecto llamado `motor_control`:

```bash
cmake --preset clang-debug \
    -DMOTOR_CONTROL_ENABLE_CPPCHECK=ON
```

Después:

```bash
cmake --build --preset clang-debug
```

Cuando se cambian opciones de análisis, es preferible configurar con `--fresh`:

```bash
cmake --preset clang-debug \
    --fresh \
    -DMOTOR_CONTROL_ENABLE_CPPCHECK=ON
```

La opción depende del nombre del proyecto:

```text
<NOMBRE_EN_MAYÚSCULAS>_ENABLE_CPPCHECK
```

Ejemplo para `serial_protocol`:

```text
SERIAL_PROTOCOL_ENABLE_CPPCHECK
```

---

# 14. Formato con clang-format

## Comprobar formato sin modificar archivos

```bash
bash scripts/format.sh --check
```

Si algún archivo no cumple `.clang-format`, el comando termina con error.

## Aplicar formato

```bash
bash scripts/format.sh --apply
```

El script busca archivos C y C++ en:

```text
include/
src/
tests/
```

Flujo recomendado antes de guardar cambios:

```bash
bash scripts/format.sh --apply
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

---

# 15. Verificación con GCC

Aunque Clang sea el compilador principal, conviene compilar también con GCC.

```bash
bash scripts/configure.sh gcc-debug
bash scripts/build.sh gcc-debug
bash scripts/test.sh gcc-debug
```

Equivalente:

```bash
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
```

GCC puede encontrar advertencias o incompatibilidades que Clang no detecta.

Ejecuta esta configuración:

- Antes de publicar una versión.
- Al utilizar características nuevas de C++.
- Después de modificar templates.
- Cuando el proyecto deba ser portable entre compiladores.

---

# 16. Cobertura de código

El preset `gcc-coverage` activa instrumentación de cobertura.

## Compilar y ejecutar las pruebas

```bash
bash scripts/configure.sh gcc-coverage
bash scripts/build.sh gcc-coverage
bash scripts/test.sh gcc-coverage
```

Equivalente:

```bash
cmake --preset gcc-coverage
cmake --build --preset gcc-coverage
ctest --preset gcc-coverage
```

## Crear un reporte HTML

Desde la raíz del proyecto:

```bash
gcovr \
    --root . \
    --filter include \
    --filter src \
    --exclude tests \
    --html-details \
    --output coverage.html \
    build/gcc-coverage
```

Abre:

```text
coverage.html
```

La cobertura indica qué código fue ejecutado, pero no demuestra que el comportamiento sea correcto.

Metas razonables iniciales:

```text
Líneas: 70 % a 85 %
Código crítico: cobertura más alta
Ramas: revisar casos no ejercitados
```

---

# 17. Release estática

La biblioteca es estática por defecto.

```bash
bash scripts/configure.sh release
bash scripts/build.sh release
```

Equivalente:

```bash
cmake --preset release
cmake --build --preset release
```

Características:

- Optimizaciones de Release.
- Pruebas deshabilitadas.
- GoogleTest no se instala.
- `BUILD_SHARED_LIBS=OFF`.

No existe un preset de pruebas para `release`, porque las pruebas están desactivadas.

---

# 18. Release compartida

Para producir una biblioteca compartida:

```bash
bash scripts/configure.sh shared-release
bash scripts/build.sh shared-release
```

Equivalente:

```bash
cmake --preset shared-release
cmake --build --preset shared-release
```

Resultados típicos:

```text
Linux:   libmotor_control.so
Windows: motor_control.dll y motor_control.lib
macOS:   libmotor_control.dylib
```

La API pública debe utilizar la macro generada de exportación:

```cpp
class MOTOR_CONTROL_EXPORT Controller
{
};
```

---

# 19. MSVC Debug

En Windows con Visual Studio 2022:

```powershell
cmake --preset msvc-debug
cmake --build --preset msvc-debug
ctest --preset msvc-debug
```

El preset utiliza:

```text
Visual Studio 17 2022
x64
Debug
```

Los scripts Bash pueden utilizarse desde WSL o Git Bash, pero en PowerShell es más directo ejecutar CMake.

---

# 20. Opciones CMake disponibles

Para un proyecto `motor_control` se generan las siguientes opciones:

```text
MOTOR_CONTROL_BUILD_TESTS
MOTOR_CONTROL_WARNINGS_AS_ERRORS
MOTOR_CONTROL_ENABLE_CLANG_TIDY
MOTOR_CONTROL_ENABLE_CPPCHECK
MOTOR_CONTROL_ENABLE_ASAN
MOTOR_CONTROL_ENABLE_UBSAN
MOTOR_CONTROL_ENABLE_TSAN
MOTOR_CONTROL_ENABLE_COVERAGE
```

Ejemplo para tratar warnings como errores:

```bash
cmake --preset clang-debug \
    --fresh \
    -DMOTOR_CONTROL_WARNINGS_AS_ERRORS=ON
```

Ejemplo para desactivar pruebas:

```bash
cmake --preset clang-debug \
    --fresh \
    -DMOTOR_CONTROL_BUILD_TESTS=OFF \
    -DVCPKG_MANIFEST_FEATURES=
```

Debe vaciarse también `VCPKG_MANIFEST_FEATURES` para evitar instalar GoogleTest innecesariamente.

---

# 21. Pruebas unitarias

Las pruebas están en:

```text
tests/
```

El target de pruebas se crea mediante GoogleTest.

Ejemplo:

```cpp
#include <motor_control/controller.hpp>

#include <gtest/gtest.h>

TEST(ControllerTest, StartsStopped)
{
    const motor_control::Controller controller;

    EXPECT_FALSE(controller.is_running());
}
```

## Añadir archivos de prueba

Modifica:

```text
tests/CMakeLists.txt
```

Ejemplo:

```cmake
add_executable(${PROJECT_NAME}_tests
    test_motor_control.cpp
    test_controller.cpp
    test_parser.cpp
)
```

## Regla importante

CTest no compila automáticamente.

Siempre ejecuta primero:

```bash
cmake --build --preset clang-debug
```

y después:

```bash
ctest --preset clang-debug
```

---

# 22. vcpkg

vcpkg se instala localmente en:

```text
.tools/vcpkg/
```

Esto evita depender de una instalación global.

## Preparar vcpkg

```bash
bash scripts/bootstrap_vcpkg.sh
```

El script:

1. Clona vcpkg si no existe.
2. Permite fijar un commit.
3. Ejecuta el bootstrap.
4. Deshabilita las métricas.

## Fijar un commit de vcpkg

```bash
VCPKG_COMMIT=<hash-del-commit> \
    bash scripts/bootstrap_vcpkg.sh
```

Esto mejora la reproducibilidad.

No escribas literalmente `<hash-del-commit>`; utiliza un commit real.

## Manifiesto

Las dependencias se declaran en:

```text
vcpkg.json
```

La feature `tests` contiene GoogleTest:

```json
{
  "features": {
    "tests": {
      "dependencies": [
        "gtest"
      ]
    }
  }
}
```

Los presets Debug habilitan esta feature mediante:

```text
VCPKG_MANIFEST_FEATURES=tests
```

Los presets Release la deshabilitan.

---

# 23. Añadir dependencias con vcpkg

## Añadir un port

Ejemplo con fmt:

```bash
.tools/vcpkg/vcpkg add port fmt
```

Formatear el manifiesto:

```bash
.tools/vcpkg/vcpkg format-manifest vcpkg.json
```

Después, en `CMakeLists.txt`:

```cmake
find_package(fmt CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        fmt::fmt
)
```

Reconfigura y compila:

```bash
bash scripts/configure.sh clang-debug
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

## Dependencias privadas y públicas

### `PRIVATE`

Usa `PRIVATE` cuando la dependencia solo aparece en archivos `.cpp`:

```cmake
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        fmt::fmt
)
```

### `PUBLIC`

Usa `PUBLIC` cuando tipos o headers de la dependencia aparecen en la API pública:

```cmake
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Boost::headers
)
```

### `INTERFACE`

Usa `INTERFACE` cuando la dependencia solo debe propagarse a consumidores:

```cmake
target_link_libraries(${PROJECT_NAME}
    INTERFACE
        alguna_dependencia
)
```

Regla práctica:

> Si un tipo de una dependencia aparece en un header público, probablemente esa dependencia debe ser `PUBLIC`.

---

# 24. Dependencias en el paquete instalado

Cuando la biblioteca instalada tiene dependencias externas, actualiza:

```text
cmake/ProjectConfig.cmake.in
```

Ejemplo con fmt:

```cmake
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

find_dependency(fmt CONFIG)

include("${CMAKE_CURRENT_LIST_DIR}/motor_controlTargets.cmake")

check_required_components(motor_control)
```

Ejemplo con Boost.System:

```cmake
include(CMakeFindDependencyMacro)

find_dependency(Boost CONFIG COMPONENTS system)
```

Esto es especialmente importante para:

- Dependencias `PUBLIC`.
- Bibliotecas estáticas.
- Targets exportados que enlazan otros targets.

Sin `find_dependency()`, el consumidor puede encontrar tu paquete pero no sus dependencias.

---

# 25. Instalar la biblioteca

## Instalación Release estática

```bash
bash scripts/configure.sh release
bash scripts/install_local.sh release
```

`install_local.sh` compila e instala.

Ruta predeterminada:

```text
out/install/release
```

## Elegir otra ruta

```bash
bash scripts/install_local.sh \
    release \
    "$HOME/.local/motor-control"
```

## Instalar la versión compartida

```bash
bash scripts/configure.sh shared-release
bash scripts/install_local.sh shared-release
```

La instalación contiene normalmente:

```text
include/
lib/
lib/cmake/nombre_del_proyecto/
```

## MSVC

Con generadores multiconfiguración, instala manualmente:

```powershell
cmake --build --preset msvc-debug

cmake --install build/msvc-debug `
    --config Debug `
    --prefix out/install/msvc-debug
```

---

# 26. Consumir la biblioteca desde otro proyecto

Supongamos que `motor_control` está instalada en:

```text
/home/usuario/.local/motor-control
```

Proyecto consumidor:

```cmake
cmake_minimum_required(VERSION 3.28)

project(aplicacion LANGUAGES CXX)

find_package(motor_control CONFIG REQUIRED)

add_executable(aplicacion
    main.cpp
)

target_link_libraries(aplicacion
    PRIVATE
        motor_control::motor_control
)
```

Configura indicando el prefijo:

```bash
cmake \
    -S . \
    -B build \
    -G Ninja \
    -DCMAKE_PREFIX_PATH="$HOME/.local/motor-control"
```

Compila:

```bash
cmake --build build
```

También puede indicarse directamente el directorio del paquete:

```bash
cmake \
    -S . \
    -B build \
    -Dmotor_control_DIR="$HOME/.local/motor-control/lib/cmake/motor_control"
```

Código consumidor:

```cpp
#include <motor_control/motor_control.hpp>

int main()
{
    return motor_control::add(2, 3);
}
```

---

# 27. Versionado

La versión aparece inicialmente en tres lugares:

## `CMakeLists.txt`

```cmake
project(
    motor_control
    VERSION 0.1.0
)
```

## `vcpkg.json`

```json
{
  "version-string": "0.1.0"
}
```

## Implementación inicial

```cpp
std::string_view version() noexcept
{
    return "0.1.0";
}
```

Al publicar una versión nueva, actualiza los tres lugares.

Ejemplo:

```text
0.1.0 → 0.2.0
```

---

# 28. Limpieza

## Limpiar un preset

```bash
rm -rf build/clang-debug
```

## Limpiar todas las compilaciones

```bash
rm -rf build
```

## Reinstalar vcpkg

```bash
rm -rf .tools/vcpkg
bash scripts/bootstrap_vcpkg.sh
```

## Eliminar instalaciones locales

```bash
rm -rf out
```

Los directorios anteriores contienen archivos generados y pueden eliminarse sin afectar el código fuente.

---

# 29. Integración continua

La plantilla incluye:

```text
.github/workflows/ci.yml
```

La configuración inicial comprueba:

```text
Linux:
    clang-asan
    clang-tidy
    gcc-debug

Windows:
    msvc-debug
```

Para usar GitHub Actions debes crear y subir manualmente un repositorio:

```bash
git init
git add .
git commit -m "Initial project structure"
```

La plantilla no inicializa Git automáticamente.

Puedes ampliar la CI para incluir:

```text
clang-tsan
gcc-coverage
release
shared-release
```

---

# 30. Flujo recomendado antes de integrar cambios

## Cambios normales

```bash
bash scripts/format.sh --apply
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

## Verificación completa local

```bash
bash scripts/format.sh --check

bash scripts/configure.sh clang-debug
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug

bash scripts/configure.sh clang-asan
bash scripts/build.sh clang-asan
bash scripts/test.sh clang-asan

bash scripts/analyze.sh

bash scripts/configure.sh gcc-debug
bash scripts/build.sh gcc-debug
bash scripts/test.sh gcc-debug
```

## Cuando existe concurrencia

Añade:

```bash
bash scripts/configure.sh clang-tsan
bash scripts/build.sh clang-tsan
bash scripts/test.sh clang-tsan
```

## Antes de publicar una versión

```bash
bash scripts/format.sh --check

bash scripts/configure.sh clang-debug
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug

bash scripts/configure.sh clang-asan
bash scripts/build.sh clang-asan
bash scripts/test.sh clang-asan

bash scripts/configure.sh clang-tsan
bash scripts/build.sh clang-tsan
bash scripts/test.sh clang-tsan

bash scripts/analyze.sh

bash scripts/configure.sh gcc-debug
bash scripts/build.sh gcc-debug
bash scripts/test.sh gcc-debug

bash scripts/configure.sh release
bash scripts/build.sh release
bash scripts/install_local.sh release
```

No es obligatorio ejecutar TSan si el proyecto no utiliza concurrencia.

---

# 31. Resumen rápido de comandos

## Desarrollo normal

```bash
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

## Reconfigurar desde cero

```bash
bash scripts/configure.sh clang-debug
```

## ASan + UBSan

```bash
bash scripts/configure.sh clang-asan
bash scripts/build.sh clang-asan
bash scripts/test.sh clang-asan
```

## ThreadSanitizer

```bash
bash scripts/configure.sh clang-tsan
bash scripts/build.sh clang-tsan
bash scripts/test.sh clang-tsan
```

## clang-tidy

```bash
bash scripts/analyze.sh
```

## Formato

```bash
bash scripts/format.sh --check
bash scripts/format.sh --apply
```

## GCC

```bash
bash scripts/configure.sh gcc-debug
bash scripts/build.sh gcc-debug
bash scripts/test.sh gcc-debug
```

## Cobertura

```bash
bash scripts/configure.sh gcc-coverage
bash scripts/build.sh gcc-coverage
bash scripts/test.sh gcc-coverage
```

## Release estática

```bash
bash scripts/configure.sh release
bash scripts/build.sh release
```

## Release compartida

```bash
bash scripts/configure.sh shared-release
bash scripts/build.sh shared-release
```

## Instalar

```bash
bash scripts/install_local.sh release
```

---

# 32. Errores comunes

## El target `test` está reservado

Mensaje:

```text
The target name "test" is reserved when CTest testing is enabled.
```

Causa:

```text
El proyecto se llamó test.
```

Solución:

Crea el proyecto con otro nombre descriptivo.

## No se encuentra vcpkg

Mensaje relacionado con:

```text
.tools/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Solución:

```bash
bash scripts/bootstrap_vcpkg.sh
```

## No se encuentra Ninja

```bash
sudo apt install ninja-build
```

## No se encuentra Clang

```bash
sudo apt install clang
```

## CMake es demasiado antiguo

Comprueba:

```bash
cmake --version
```

La plantilla requiere CMake 3.28 o posterior.

## Una dependencia añadida no se encuentra

Después de modificar `vcpkg.json`:

```bash
bash scripts/configure.sh clang-debug
```

Comprueba además:

1. Que la dependencia esté en `vcpkg.json`.
2. Que exista un `find_package(...)`.
3. Que el target enlazado tenga el nombre correcto.
4. Que vcpkg haya terminado sin errores.

## CTest no encuentra el ejecutable

Primero compila:

```bash
bash scripts/build.sh clang-debug
```

Después:

```bash
bash scripts/test.sh clang-debug
```

## Cambié de compilador y aparecen errores extraños

No reutilices el mismo directorio de build.

Utiliza otro preset o elimina el directorio afectado:

```bash
rm -rf build/clang-debug
```

## Una DLL no se encuentra en Windows

La DLL debe:

- Estar junto al ejecutable.
- Estar en un directorio incluido en `PATH`.
- Copiarse o instalarse junto con el consumidor.

---

# 33. Principios de uso

1. Mantén la API pública dentro de `include/`.
2. Mantén los detalles privados dentro de `src/`.
3. Enumera explícitamente los `.cpp` en CMake.
4. Compila y prueba diariamente con `clang-debug`.
5. Ejecuta ASan/UBSan periódicamente.
6. Ejecuta TSan cuando exista concurrencia.
7. Usa clang-tidy como complemento, no como sustituto de las pruebas.
8. Verifica también con GCC y MSVC cuando corresponda.
9. Declara dependencias con vcpkg y propágalas correctamente con `PRIVATE`, `PUBLIC` o `INTERFACE`.
10. Prueba la instalación y el consumo mediante `find_package()` antes de publicar una versión.

La combinación de compilador, análisis estático, pruebas, sanitizers y compiladores alternativos ofrece varias capas complementarias de detección de errores.
