#include "command_packet.h"

#include <string.h>

_Static_assert(
    sizeof(command_packet_t) == COMMAND_PACKET_SIZE,
    "command_packet_t must stay exactly 11 bytes"
);

_Static_assert(
    sizeof(command_response_t) == COMMAND_PACKET_SIZE,
    "command_response_t must stay exactly 11 bytes"
);

static bool command_packet_has_magic(
    const uint8_t magic[COMMAND_MAGIC_SIZE]
)
{
    /*
     * Copy eight bytes so the terminating '\0' after PADAWAN is part of the
     * protocol signature. This must match the Qt viewer exactly.
     */
    return memcmp(magic, COMMAND_MAGIC, COMMAND_MAGIC_SIZE) == 0;
}

bool command_packet_is_supported_command(uint8_t command)
{
    return command == COMMAND_START
        || command == COMMAND_STOP;
}

bool command_packet_is_valid_request(const command_packet_t *packet)
{
    if (!command_packet_has_magic(packet->magic)) {
        return false;
    }

    if (packet->version != COMMAND_VERSION) {
        return false;
    }

    if (!command_packet_is_supported_command(packet->command)) {
        return false;
    }

    return true;
}

void command_response_init(
    command_response_t *response,
    uint8_t command,
    uint8_t status
)
{
    /*
     * Response packets use the same fixed signature as requests, including the
     * trailing '\0' byte in the 8-byte magic field.
     */
    memcpy(response->magic, COMMAND_MAGIC, COMMAND_MAGIC_SIZE);

    response->version = COMMAND_VERSION;
    response->command = command;
    response->status = status;
}
