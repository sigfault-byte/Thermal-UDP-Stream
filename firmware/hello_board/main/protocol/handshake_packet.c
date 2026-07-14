#include "handshake_packet.h"

#include <stddef.h>
#include <string.h>

_Static_assert(
    sizeof(handshake_packet_t) == 10,
    "handshake_packet_t must stay exactly 10 bytes"
);

void handshake_packet_init(handshake_packet_t *packet)
{
    /*
     * Copy eight bytes so the trailing '\0' is part of the magic field.
     * The wire value starts as: 50 41 44 41 57 41 4E 00.
     */
    memcpy(packet->magic, HANDSHAKE_MAGIC, HANDSHAKE_MAGIC_SIZE);

    /* Version 1 is the first and current handshake packet format. */
    packet->version = HANDSHAKE_VERSION;

    /* Type 1 marks this packet as the discovery/handshake announcement. */
    packet->type = HANDSHAKE_TYPE_DISCOVERY;
}

void handshake_packet_encode(handshake_packet_t *packet)
{
    /*
     * Treat the packed structure as a byte array because the XOR is defined
     * over the complete 10-byte packet, not over individual C fields.
     */
    uint8_t *bytes = (uint8_t *)packet;

    /*
     * The secret is repeated across the packet:
     *
     *   byte[0] ^= 'p'
     *   byte[1] ^= 'a'
     *   ...
     *   byte[7] ^= 'p'
     *   byte[8] ^= 'a'
     *   byte[9] ^= 'd'
     *
     * This is intentionally light obfuscation, not security.
     */
    for (size_t i = 0; i < sizeof(*packet); i++) {
        bytes[i] ^= HANDSHAKE_SECRET[i % HANDSHAKE_SECRET_SIZE];
    }
}
