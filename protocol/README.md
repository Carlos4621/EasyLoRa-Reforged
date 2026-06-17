# EasyLoRa protocol

En esta carpeta se recopilan los headers y fuentes en comun para la API y firmware del protocolo de envio y recepcion de informacion.

## Formato binario del frame

`FrameCodec` convierte un `Frame` a bytes en formato raw y agrega el CRC. `ProtocolCodec` toma ese frame raw, le aplica COBS/R para que pueda viajar por un transporte serial y agrega el delimitador final `0x00`.

Frame raw antes de COBS/R:

| Offset | Tamano | Campo | Formato |
| --- | ---: | --- | --- |
| 0 | 1 byte | `version` | `Frame::Actual_Frame_Version` actual: `1` |
| 1 | 1 byte | `kind` | Valor numerico de `PackageKind` |
| 2 | 1 byte | `flags` | Preservado por el protocolo |
| 3 | 1 byte | `reserved` | Preservado por el protocolo |
| 4 | 2 bytes | `seq` | Big endian |
| 6 | 1 byte | `type` | Valor numerico de `MessageType` |
| 7 | N bytes | `payload` | Bytes opacos para la capa de frame |
| 7 + N | 2 bytes | `crc` | CRC16-CCITT-FALSE en big endian |

El CRC se calcula sobre todos los bytes anteriores al campo `crc`: header completo mas payload. El payload maximo aceptado por `FrameCodec::encode` es `FrameCodec::Max_Frame_Payload_Size`.

En decode, la validacion de CRC ocurre antes de confiar en los campos del header. Si el CRC no coincide, `FrameDecoder` y `ProtocolDecoder` devuelven `ProtocolErrors::CRCMismatch` aunque `version`, `kind` o `type` tambien contengan valores invalidos.

`ProtocolCodec::encode` produce:

```text
COBSR(version | kind | flags | reserved | seq_hi | seq_lo | type | payload | crc_hi | crc_lo) | 0x00
```

`ProtocolDecoder` espera recibir el frame COBS/R con su delimitador final `0x00`. Si una capa de stream separa frames consecutivos al encontrar ese byte, debe conservarlo en el span que entrega al decoder.

## Buffers y lifetime

`Frame::payload` es un `std::span<const uint8_t>`: el protocolo no copia ni administra la memoria del payload. El buffer apuntado por `payload` debe seguir vivo mientras se use el `Frame`.

En encode:

- `FrameCodec::minimumOutputBufferSize(frame)` devuelve `Frame::Header_Size + frame.payload.size() + Frame::CRC_Size`.
- `ProtocolCodec::minimumFrameBytesBufferSize(frame)` devuelve el tamano del frame raw intermedio.
- `ProtocolCodec::minimumOutputBufferSize(frame)` devuelve el tamano maximo necesario para COBS/R mas el delimitador final.

En decode:

- `ProtocolDecoder::minimumFrameBytesBufferSize(inputSize)` recibe el tamano del input COBS/R con delimitador y devuelve un tamano seguro para el buffer raw intermedio.
- `ProtocolDecoder::minimumPayloadBufferSize(inputSize)` recibe el tamano del input COBS/R con delimitador y devuelve un tamano conservador para `payloadInFrame`.
- `FrameDecoder::minimumPayloadBufferSize(rawSize)` recibe el tamano raw, no el tamano COBS/R.

`ProtocolDecoder::decode(ProtocolDecoderBuffers{...})` usa `frameBytes` como scratch buffer para el frame raw y copia el payload valido en `payloadInFrame`. `frameBytes` y `payloadInFrame` deben ser buffers distintos.

## Errores

| Error | Capa principal | Condicion |
| --- | --- | --- |
| `OutputBufferTooSmall` | encode/decode | El buffer de salida no alcanza para el resultado solicitado. |
| `CRCMismatch` | frame/protocol decode | El CRC recibido no coincide con el CRC calculado. Tiene prioridad sobre errores semanticos del header. |
| `FramePayloadTooSmall` | frame/protocol decode | `payloadInFrame` no alcanza para copiar el payload decodificado. |
| `FramePayloadTooLong` | frame/protocol encode | El payload de entrada supera `FrameCodec::Max_Frame_Payload_Size`. |
| `InvalidPackageKind` | frame/protocol encode/decode | `kind` no corresponde a ningun valor de `PackageKind`. |
| `InvalidMessageType` | frame/protocol encode/decode | `type` no corresponde a ningun valor de `MessageType`. |
| `COBSREncodeError` | COBS/R encode | La libreria COBS/R rechazo la codificacion. |
| `COBSRDecodeError` | COBS/R decode | El input COBS/R es invalido, por ejemplo contiene `0x00`. |
| `SameBufferError` | encode/decode | Dos buffers se solapan en una operacion donde el solapamiento no es seguro. |
| `EmptyInputBuffer` | COBS/R decode | Se intento decodificar un input vacio. |
| `IncoherentFrame` | reservado | Error reservado para contratos de frame incoherentes. |
| `CodificationError` | reservado | Error reservado para fallos de codificacion no clasificados. |
| `HandlerWithIncorrectType` | reservado | Error reservado para dispatch/handlers de otro nivel. |
| `FrameVersionMismatch` | frame/protocol decode | `version` no coincide con `Frame::Actual_Frame_Version`. |
| `InputBufferTooLong` | frame/protocol decode | El frame raw supera `FrameDecoder::Max_Input_Buffer_Size`. |
| `InputBufferTooSmall` | frame/protocol decode | El frame raw es menor que `FrameDecoder::Min_Raw_Buffer_Size`. |

## Politica de `seq`

`seq` es un identificador de correlacion sin estado dentro de la capa de protocolo.

`ProtocolCodec` solo serializa el valor recibido en `Frame::seq` y `ProtocolDecoder` solo lo entrega al usuario despues de validar y decodificar el frame. La capa de protocolo no guarda requests pendientes, no decide si una respuesta corresponde a una operacion activa, no maneja timeouts y no filtra duplicados.

La correlacion entre `Request`, `Response` y `Error` pertenece al dispositivo o a la capa de aplicacion que usa el protocolo. Esa capa debe decidir como generar secuencias, guardar operaciones pendientes, aceptar o rechazar respuestas tardias, manejar duplicados y aplicar reintentos.

Reglas del campo:

- `Request`: usa un `seq` elegido por el emisor.
- `Response`: debe copiar el `seq` del `Request` que responde.
- `Error`: si responde a un `Request`, debe copiar el `seq` de ese `Request`.
- `Event`: puede usar `seq` para trazabilidad, pero no requiere correlacion con un `Request`.

El campo es de 16 bits y se codifica en big endian dentro del frame. Si el emisor usa un contador, el wrap-around ocurre naturalmente de `0xFFFF` a `0x0000`; la decision de aceptar o ignorar valores repetidos despues del wrap-around corresponde a la capa de aplicacion.

## Build

Dependencias requeridas:

- CMake 3.20 o superior.
- Compilador con C++23.
- COBS instalado como libreria C (`cobs.h` y `libcobs`).
- CRCpp instalado como headers (`CRC.h` o `CRCpp.h`).
- Protobuf y `protoc`.
- nanopb. Si `NANOPB_ROOT` no se define, CMake descarga nanopb 0.4.8 con `FetchContent`.

Politica actual de dependencias externas:

- COBS y CRCpp son dependencias explicitas del entorno de build; usar `COBS_ROOT` y `CRCPP_ROOT` cuando no esten en rutas del sistema.
- Protobuf/protoc tambien se resuelven desde el entorno para evitar descargar toolchains de generacion en configure.
- nanopb mantiene el fallback con `FetchContent` porque el generador y las fuentes C se integran al build local y ya estan fijados a la version 0.4.8.

Los esquemas compartidos viven en `../messages`. Cada proyecto genera sus clases
en su propio directorio de build; CMake conserva los archivos existentes y
vuelve a ejecutar `protoc` solamente si falta un output o cambia un `.proto` o
su archivo `.options`.

En `protocol`, `EASYLORA_ENABLE_PROTOBUF` y `EASYLORA_ENABLE_NANOPB` controlan
qué variante se genera. Ambas están activadas al compilar `protocol` de forma
independiente; API desactiva nanopb y firmware desactiva ambas para generar su
propia variante nanopb con el toolchain de Pico.

Presets utiles:

```sh
cmake --preset dev-tests
cmake --build --preset dev-tests
ctest --preset dev-tests
```

```sh
cmake --preset strict
cmake --build --preset strict
ctest --preset strict
```

```sh
cmake --preset sanitize
cmake --build --preset sanitize
ctest --preset sanitize
```

Para instalar el paquete CMake:

```sh
cmake --install build-dev-tests --prefix /tmp/easylora-protocol
```

Un consumidor CMake puede usar:

```cmake
find_package(EasyLoRaProtocol CONFIG REQUIRED)
target_link_libraries(mi_target PRIVATE EasyLoRa::protocol)
```
