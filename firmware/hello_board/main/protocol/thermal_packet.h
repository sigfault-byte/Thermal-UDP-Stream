#pragma once

#include <stdint.h>

#define THERMAL_VERSION 1
#define THERMAL_TYPE_FRAME 1

#define THERMAL_WIDTH 32
#define THERMAL_HEIGHT 24
#define THERMAL_PIXELS (THERMAL_WIDTH * THERMAL_HEIGHT)

typedef struct __attribute__((packed)) {
    uint8_t magic[8];
    uint8_t version;
    uint8_t type;
    uint32_t frame_id;
    uint32_t timestamp_ms;
} padawan_header_t;

typedef struct __attribute__((packed)) {
    padawan_header_t header;
    uint8_t pixels[THERMAL_PIXELS];
} thermal_packet_t;

void thermal_packet_init(
    thermal_packet_t *packet,
    uint32_t frame_id,
    uint32_t timestamp_ms,
    const uint8_t pixels[THERMAL_PIXELS]
);
void thermal_packet_init(
    thermal_packet_t *packet,
    uint32_t frame_id,
    uint32_t timestamp_ms,
    const uint8_t pixels[THERMAL_PIXELS]
);
