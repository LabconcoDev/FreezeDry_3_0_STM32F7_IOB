#ifndef FRAMER_H_
#define FRAMER_H_

#include "protocol_defs.h"
#include <stddef.h>

typedef void (*fd_packet_handler_t)(const fd_packet_t *pkt);

typedef enum {
    FD_STATE_SOF_L,
    FD_STATE_SOF_C,
    FD_STATE_LEN,
    FD_STATE_PAYLOAD,
    FD_STATE_CSUM_LSB,
    FD_STATE_CSUM_MSB,
} fd_framer_state_t;

/* Callback when a valid packet is found */
typedef void (*fd_packet_handler_t)(const fd_packet_t *pkt);

typedef struct {
    fd_framer_state_t state;
    uint8_t expected_len;
    uint8_t idx;
    uint16_t csum;
    uint8_t buf[FD_MAX_PAYLOAD_SIZE];
    fd_packet_handler_t callback;
} fd_framer_t;

void fd_framer_init(fd_framer_t *framer, fd_packet_handler_t cb);
void fd_framer_feed_bytes(fd_framer_t *framer, const uint8_t *data, size_t len);
size_t fd_framer_serialize(uint8_t *out_buf, size_t max_len,
                           fd_class_t cls, fd_type_t type, uint8_t action,
                           const uint8_t *data, uint8_t data_len);

#endif /* FRAMER_H_ */