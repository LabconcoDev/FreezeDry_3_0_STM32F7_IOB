#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "protocol_defs.h"
#include <stddef.h>

/*
 * Builds a complete frame ready for DMA transmission.
 * Returns the total number of bytes written to `out_buf`.
 * Returns 0 if the max_len is too small.
 */
size_t fd_serializer_build_frame(uint8_t *out_buf, size_t max_len,
                                 uint8_t cls, uint8_t type, uint8_t action,
                                 const uint8_t *data, uint8_t data_len);

#endif /* SERIALIZER_H */