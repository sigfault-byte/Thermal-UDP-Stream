#pragma once

#include <stdbool.h>
#include <stdint.h>

#define COMMAND_MAGIC "PADAWAN"
#define COMMAND_MAGIC_SIZE 8
#define COMMAND_VERSION 1

#define COMMAND_PACKET_SIZE 11

#define COMMAND_START 1
#define COMMAND_STOP 2

#define COMMAND_STATUS_OK 1
#define COMMAND_STATUS_ERROR 2
#define COMMAND_STATUS_INVALID_VALUE 3

/*
 * Request packet sent by the Qt viewer over TCP port 5006.
 *
 * Layout is exactly 11 bytes:
 *
 *   magic[8] -> "PADAWAN\0"
 *   version  -> 0x01
 *   command  -> START, STOP, future commands
 *   value    -> command parameter, or 0 when unused
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[COMMAND_MAGIC_SIZE];
    uint8_t version;
    uint8_t command;
    uint8_t value;
} command_packet_t;

/*
 * Response packet sent by the ESP32 over the same TCP connection.
 *
 * Layout is exactly 11 bytes:
 *
 *   magic[8] -> "PADAWAN\0"
 *   version  -> 0x01
 *   command  -> echoed request command
 *   status   -> OK, ERROR, INVALID_VALUE
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[COMMAND_MAGIC_SIZE];
    uint8_t version;
    uint8_t command;
    uint8_t status;
} command_response_t;

/*
 * Return true only for commands the firmware recognizes in this v1 protocol.
 */
bool command_packet_is_supported_command(uint8_t command);

/*
 * Validate the fixed packet fields and command byte.
 * The value byte is intentionally accepted and ignored for now.
 */
bool command_packet_is_valid_request(const command_packet_t *packet);

/*
 * Fill a response packet with the shared magic/version, echoed command, and
 * status byte selected by the TCP command server.
 */
void command_response_init(
    command_response_t *response,
    uint8_t command,
    uint8_t status
);
