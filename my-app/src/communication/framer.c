#include "framer.h"
#include <string.h>

static inline uint16_t calc_checksum(uint8_t len, const uint8_t *buf)
{
    uint16_t sum = len;
    for (uint8_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum;
}

void fd_framer_init(fd_framer_t *framer, fd_packet_handler_t cb)
{
    framer->state = FD_STATE_SOF_L;
    framer->expected_len = 0;
    framer->idx = 0;
    framer->csum = 0;
    framer->callback = cb;
}

void fd_framer_feed_bytes(fd_framer_t *framer, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t b = data[i];

        switch (framer->state) {
        case FD_STATE_SOF_L:
            if (b == FD_SOF_L) framer->state = FD_STATE_SOF_C;
            break;

        case FD_STATE_SOF_C:
            if (b == FD_SOF_C) {
                framer->state = FD_STATE_LEN;
            } else {
                framer->state = (b == FD_SOF_L) ? FD_STATE_SOF_C : FD_STATE_SOF_L;
            }
            break;

        case FD_STATE_LEN:
            if (b == 0 || b > FD_MAX_PAYLOAD_SIZE) {
                framer->state = FD_STATE_SOF_L;
            } else {
                framer->expected_len = b;
                framer->idx = 0;
                framer->state = FD_STATE_PAYLOAD;
            }
            break;

        case FD_STATE_PAYLOAD:
            framer->buf[framer->idx++] = b;
            if (framer->idx >= framer->expected_len) {
                framer->state = FD_STATE_CSUM_LSB;
            }
            break;

        case FD_STATE_CSUM_LSB:
            framer->csum = b;
            framer->state = FD_STATE_CSUM_MSB;
            break;

        case FD_STATE_CSUM_MSB:
            framer->csum |= ((uint16_t)b << 8);
            if (calc_checksum(framer->expected_len, framer->buf) == framer->csum) {
                if (framer->callback) {
                    fd_packet_t pkt = {
                        .len = framer->expected_len,
                        .payload = (fd_payload_t *)framer->buf
                    };
                    framer->callback(&pkt);
                }
            }
            framer->state = FD_STATE_SOF_L;
            break;
        }
    }
}

// size_t fd_framer_serialize(uint8_t *out, size_t max_len,
//                            fd_class_t cls, fd_type_t type, uint8_t action,
//                            const uint8_t *data, uint8_t data_len)
// {
//     uint8_t payload_len = 3 + data_len;
//     size_t total = FD_OVERHEAD_SIZE + payload_len;

//     if (total > max_len) {
//         return 0;
//     }

//     out[0] = FD_SOF_L;
//     out[1] = FD_SOF_C;
//     out[2] = payload_len;
//     out[3] = (uint8_t)cls;
//     out[4] = (uint8_t)type;
//     out[5] = action;

//     if (data && data_len > 0) {
//         memcpy(&out[6], data, data_len);
//     }

//     uint16_t csum = calc_checksum(payload_len, &out[3]);
//     out[3 + payload_len] = (uint8_t)(csum & 0xFF);
//     out[4 + payload_len] = (uint8_t)((csum >> 8) & 0xFF);

//     return total;
// }