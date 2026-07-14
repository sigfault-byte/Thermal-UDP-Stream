#pragma once

#include <stdint.h>

#define THERMAL_VERSION 2
#define THERMAL_TYPE_FRAME 1

#define THERMAL_WIDTH 32
#define THERMAL_HEIGHT 24
#define THERMAL_PIXELS (THERMAL_WIDTH * THERMAL_HEIGHT)
#define THERMAL_HEADER_SIZE 19
#define THERMAL_PACKET_SIZE (THERMAL_HEADER_SIZE + THERMAL_PIXELS)

/*
 * Thermal frame header, protocol version 2.
 *
 * The quantization byte lives after timestamp_ms so frame_id and timestamp_ms
 * keep the same offsets they had in protocol version 1.
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[8];
    uint8_t version;
    uint8_t type;
    uint32_t frame_id;
    uint32_t timestamp_ms;
    uint8_t quantization_mode;
} padawan_header_t;

typedef struct __attribute__((packed)) {
    padawan_header_t header;
    uint8_t pixels[THERMAL_PIXELS];
} thermal_packet_t;

void thermal_packet_init(
    thermal_packet_t *packet,
    uint32_t frame_id,
    uint32_t timestamp_ms,
    uint8_t quantization_mode,
    const uint8_t pixels[THERMAL_PIXELS]
);
