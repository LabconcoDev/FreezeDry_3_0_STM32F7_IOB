#ifndef PARSER_H
#define PARSER_H

#include "protocol_defs.h"

typedef enum {
    INTERFACE_APP = 0,
    INTERFACE_ACCESSORY_IOB = 1
} fd_interface_t;

/* Takes a validated packet from the framer and routes it */
void fd_parser_process_packet(fd_interface_t source_if, const fd_packet_t *pkt);

#endif /* PARSER_H */