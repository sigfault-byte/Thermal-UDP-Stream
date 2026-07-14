#include "utils/quantization.h"

typedef struct {
    float minimum_celsius;
    float maximum_celsius;
} quantization_range_t;

bool quantization_mode_is_valid(uint8_t mode)
{
    return mode == QUANTIZATION_MODE_10_45
        || mode == QUANTIZATION_MODE_20_60
        || mode == QUANTIZATION_MODE_0_100;
}

static quantization_range_t quantization_range_for_mode(uint8_t mode)
{
    if (mode == QUANTIZATION_MODE_20_60) {
        return (quantization_range_t) {
            .minimum_celsius = 20.0f,
            .maximum_celsius = 60.0f,
        };
    }

    if (mode == QUANTIZATION_MODE_0_100) {
        return (quantization_range_t) {
            .minimum_celsius = 0.0f,
            .maximum_celsius = 100.0f,
        };
    }

    return (quantization_range_t) {
        .minimum_celsius = 10.0f,
        .maximum_celsius = 45.0f,
    };
}

static uint8_t temp_to_byte(
    float t,
    quantization_range_t range
)
{
    if (t < range.minimum_celsius) return 0;
    if (t > range.maximum_celsius) return 255;

    return 1 + (uint8_t)(
        (t - range.minimum_celsius)
        * 253.0f
        / (range.maximum_celsius - range.minimum_celsius)
    );
}

void temps_to_pixels(
    const float temperatures[THERMAL_PIXELS],
    uint8_t mode,
    uint8_t pixels[THERMAL_PIXELS]
)
{
    quantization_range_t range = quantization_range_for_mode(mode);

    for (int i = 0; i < THERMAL_PIXELS; i++) {
        pixels[i] = temp_to_byte(temperatures[i], range);
    }
}
