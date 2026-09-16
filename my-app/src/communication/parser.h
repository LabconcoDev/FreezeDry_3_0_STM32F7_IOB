#ifndef PARSER_H
#define PARSER_H

#include "protocol_defs.h"

/* Takes a validated packet from the framer and routes it */
void fd_parser_process_packet(const fd_packet_t *pkt);

#endif /* PARSER_H */