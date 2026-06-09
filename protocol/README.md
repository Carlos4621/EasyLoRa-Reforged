# EasyLoRa protocol

En esta carpeta se recopilan los headers y fuentes en comun para la API y firmware del protocolo de envio y recepcion de informacion.

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
