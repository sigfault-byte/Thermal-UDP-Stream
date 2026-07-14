#include "thermal_packet.h"

#include <string.h>

_Static_assert(
    sizeof(padawan_header_t) == THERMAL_HEADER_SIZE,
    "padawan_header_t must stay exactly 19 bytes"
);

_Static_assert(
    sizeof(thermal_packet_t) == THERMAL_PACKET_SIZE,
    "thermal_packet_t must stay exactly 787 bytes"
);

void thermal_packet_init(
    thermal_packet_t *packet,
    uint32_t frame_id,
    uint32_t timestamp_ms,
    uint8_t quantization_mode,
    const uint8_t pixels[THERMAL_PIXELS]
)
{
    memcpy(packet->header.magic, "PADAWAN", 8);

    packet->header.version = THERMAL_VERSION;
    packet->header.type = THERMAL_TYPE_FRAME;
    packet->header.frame_id = frame_id;
    packet->header.timestamp_ms = timestamp_ms;
    packet->header.quantization_mode = quantization_mode;

    memcpy(packet->pixels, pixels, THERMAL_PIXELS);
}
