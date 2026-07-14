#pragma once

#include <stdint.h>

#define HANDSHAKE_MAGIC "PADAWAN"
#define HANDSHAKE_MAGIC_SIZE 8
#define HANDSHAKE_VERSION 1
#define HANDSHAKE_TYPE_DISCOVERY 1
#define HANDSHAKE_SECRET "padawan"
#define HANDSHAKE_SECRET_SIZE 7

/*
 * This is the tiny discovery/handshake payload.
 *
 * It is exactly 10 bytes on the wire before XOR obfuscation:
 *
 *   magic[8] -> "PADAWAN\0"
 *   version  -> 0x01
 *   type     -> 0x01
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[HANDSHAKE_MAGIC_SIZE];
    uint8_t version;
    uint8_t type;
} handshake_packet_t;

/*
 * Fill a packet with the cleartext handshake values.
 * Call handshake_packet_encode() before sending it over the network.
 */
void handshake_packet_init(handshake_packet_t *packet);

/*
 * Apply the playful repeating-key XOR obfuscation in place.
 * Calling this function a second time with the same packet decodes it again.
 */
void handshake_packet_encode(handshake_packet_t *packet);
