# EasyLoRa messages

Esta carpeta contiene los esquemas Protobuf compartidos por `protocol`,
`firmware` y `api`.

`EasyLoRaMessages.cmake` expone dos helpers:

- `easylora_generate_protobuf`: genera las clases C++ de Protobuf.
- `easylora_generate_nanopb`: genera las clases C de nanopb.

Ambos reciben un directorio de salida del árbol de build. Los outputs declarados
permiten que CMake genere las clases si faltan y las regenere cuando cambia el
esquema o las opciones de nanopb.
