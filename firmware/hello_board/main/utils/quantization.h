#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "protocol/thermal_packet.h"

#define QUANTIZATION_MODE_10_45 1
#define QUANTIZATION_MODE_20_60 2
#define QUANTIZATION_MODE_0_100 3

bool quantization_mode_is_valid(uint8_t mode);

void temps_to_pixels(
    const float temperatures[THERMAL_PIXELS],
    uint8_t mode,
    uint8_t pixels[THERMAL_PIXELS]
);
