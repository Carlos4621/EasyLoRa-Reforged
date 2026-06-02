#ifndef NANO_PB_CALLBACKS_HEADER
#define NANO_PB_CALLBACKS_HEADER

#include "pb_encode.h"
#include <cstring>

/// @brief Callback necesario para codificar un field de string en nanopb
[[nodiscard]]
static bool writeStringCallback(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    const char* str = static_cast<const char*>(*arg);

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }
    
    return pb_encode_string(stream, reinterpret_cast<const pb_byte_t*>(str), std::strlen(str));
}

#endif // !NANO_PB_CALLBACKS_HEADER