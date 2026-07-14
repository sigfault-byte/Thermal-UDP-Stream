#include "utils/refresh_rate.h"

bool refresh_rate_is_valid(uint8_t refresh_rate_hz)
{
    return refresh_rate_hz == REFRESH_RATE_1_HZ
        || refresh_rate_hz == REFRESH_RATE_8_HZ;
}

uint8_t refresh_rate_to_mlx90640_code(uint8_t refresh_rate_hz)
{
    if (refresh_rate_hz == REFRESH_RATE_8_HZ) {
        return MLX90640_REFRESH_CODE_8_HZ;
    }

    return MLX90640_REFRESH_CODE_1_HZ;
}
