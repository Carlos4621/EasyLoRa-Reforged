# protocol

Biblioteca C++23 creada a partir de una plantilla con CMake, Ninja, vcpkg, sanitizers, análisis estático y GoogleTest.

## Requisitos

- CMake 3.28 o posterior.
- Git.
- Ninja para los presets de Clang y GCC.
- Clang o GCC en Linux.
- Visual Studio 2022 para el preset de MSVC.
- clang-format y clang-tidy para formato y análisis.

vcpkg se instala localmente en `.tools/vcpkg` mediante el script incluido.

## Proyecto generado y configurado automáticamente

Cuando este proyecto se crea mediante el `script.sh` de la plantilla, vcpkg se prepara y CMake se configura automáticamente con el preset `clang-debug`.

El generador no inicializa un repositorio Git, no compila la biblioteca y no ejecuta las pruebas. Esas acciones quedan bajo control del desarrollador.

Para repetir manualmente la preparación de vcpkg:

```bash
bash scripts/bootstrap_vcpkg.sh
```

Para fijar una revisión concreta de vcpkg:

```bash
VCPKG_COMMIT=<commit> bash scripts/bootstrap_vcpkg.sh
```

## Desarrollo con Clang y clangd

```bash
bash scripts/configure.sh clang-debug
bash scripts/build.sh clang-debug
bash scripts/test.sh clang-debug
```

CMake genera la base de compilación directamente en:

```text
build/clang-debug/compile_commands.json
```

El archivo `.clangd` de la raíz indica a clangd que lea esa base directamente.
No se crean copias ni enlaces simbólicos. De esta forma clangd recibe también
la ruta `build/clang-debug/generated`, donde CMake crea el encabezado de
exportación de la biblioteca.

`clang-debug` se utiliza como preset de desarrollo para el editor. Configúralo
al menos una vez, incluso si después compilas o pruebas con otros presets:

```bash
bash scripts/configure.sh clang-debug
cmake --preset clang-asan
cmake --build --preset clang-asan
```

## Sanitizers

ASan y UBSan:

```bash
cmake --preset clang-asan
cmake --build --preset clang-asan
ctest --preset clang-asan
```

ThreadSanitizer:

```bash
cmake --preset clang-tsan
cmake --build --preset clang-tsan
ctest --preset clang-tsan
```

## Análisis estático

```bash
bash scripts/analyze.sh
```

## Formato

Comprobar:

```bash
bash scripts/format.sh --check
```

Aplicar:

```bash
bash scripts/format.sh --apply
```

## Biblioteca estática o compartida

La biblioteca es estática por defecto:

```bash
cmake --preset release
cmake --build --preset release
```

Para generar una biblioteca compartida:

```bash
cmake --preset shared-release
cmake --build --preset shared-release
```

## Instalar la biblioteca

```bash
bash scripts/install_local.sh release
```

Por defecto se instala en:

```text
out/install/release
```

Un proyecto consumidor puede usarla así:

```cmake
find_package(protocol CONFIG REQUIRED)

target_link_libraries(mi_aplicacion
    PRIVATE
        protocol::protocol
)
```

## API inicial

```cpp
#include <protocol/protocol.hpp>

const auto result = protocol::add(2, 3);
```

## Añadir dependencias con vcpkg

```bash
.tools/vcpkg/vcpkg add port fmt
.tools/vcpkg/vcpkg format-manifest vcpkg.json
```

Después:

```cmake
find_package(fmt CONFIG REQUIRED)
target_link_libraries(protocol PRIVATE fmt::fmt)
```

## Documentación del entorno

La guía completa se encuentra en:

```text
docs/guia_entorno_cpp_vcpkg.md
```
