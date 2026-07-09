#include "protocol/thermal_packet.h"

static uint8_t temp_to_byte(float t)
{
    if (t < 10.0f) return 0;
    if (t > 45.0f) return 255;

    return 1 + (uint8_t)((t - 10.0f) * 253.0f / 35.0f);
}

void temps_to_pixels(
    const float temperatures[THERMAL_PIXELS],
    uint8_t pixels[THERMAL_PIXELS]
)
{
    for (int i = 0; i < THERMAL_PIXELS; i++) {
        pixels[i] = temp_to_byte(temperatures[i]);
    }
}
