#include "thermal_packet.h"

#include <string.h>

void thermal_packet_init(
    thermal_packet_t *packet,
    uint32_t frame_id,
    uint32_t timestamp_ms,
    const uint8_t pixels[THERMAL_PIXELS]
)
{
    memcpy(packet->header.magic, "PADAWAN", 8);

    packet->header.version = THERMAL_VERSION;
    packet->header.type = THERMAL_TYPE_FRAME;
    packet->header.frame_id = frame_id;
    packet->header.timestamp_ms = timestamp_ms;

    memcpy(packet->pixels, pixels, THERMAL_PIXELS);
}
