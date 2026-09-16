#include "serializer.h"
#include <string.h>

static inline uint16_t calc_checksum(uint8_t len, const uint8_t *buf) {
    uint16_t sum = len;
    for (uint8_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}

size_t fd_serializer_build_frame(uint8_t *out_buf, size_t max_len,
                                 uint8_t cls, uint8_t type, uint8_t action,
                                 const uint8_t *data, uint8_t data_len) 
{
    uint8_t payload_len = 3 + data_len; // cls(1) + type(1) + action(1) + data
    size_t total_len = FD_OVERHEAD_SIZE + payload_len;

    if (total_len > max_len) {
        return 0; // Buffer too small
    }

    out_buf[0] = FD_SOF_L;
    out_buf[1] = FD_SOF_C;
    out_buf[2] = payload_len;
    out_buf[3] = cls;
    out_buf[4] = type;
    out_buf[5] = action;

    if (data != NULL && data_len > 0) {
        memcpy(&out_buf[6], data, data_len);
    }

    uint16_t csum = calc_checksum(payload_len, &out_buf[3]);
    out_buf[3 + payload_len] = (uint8_t)(csum & 0xFF);
    out_buf[4 + payload_len] = (uint8_t)((csum >> 8) & 0xFF);

    return total_len;
}