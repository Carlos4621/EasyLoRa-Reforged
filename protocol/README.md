# EasyLoRa protocol

En esta carpeta se recopilan los headers y fuentes en comun para la API y firmware del protocolo de envio y recepcion de informacion.

## Formato binario del frame

`FrameCodec` convierte un `Frame` a bytes en formato raw y agrega el CRC. `ProtocolCodec` toma ese frame raw y le aplica COBS/R para que pueda viajar por un transporte serial. El resultado de `ProtocolCodec` no incluye delimitador `0x00`.

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

`ProtocolCodec::encode` produce:

```text
COBSR(version | kind | flags | reserved | seq_hi | seq_lo | type | payload | crc_hi | crc_lo)
```

Si el transporte necesita separar frames consecutivos, el delimitador `0x00` pertenece a la capa de stream/transporte y debe agregarse fuera de `ProtocolCodec`. De la misma forma, `ProtocolDecoder` espera recibir un frame COBS/R sin ese delimitador.

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
