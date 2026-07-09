#pragma once

#include <stdint.h>

#include "protocol/thermal_packet.h"

void temps_to_pixels(
    const float temperatures[THERMAL_PIXELS],
    uint8_t pixels[THERMAL_PIXELS]
);
