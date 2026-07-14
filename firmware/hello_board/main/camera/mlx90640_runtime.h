#pragma once

#include <stdint.h>

/*
 * Runtime-safe MLX90640 helpers.
 *
 * After boot, more than one FreeRTOS task can want the camera:
 *
 *   - UDP sender task reads frame data while streaming.
 *   - TCP command task writes the refresh-rate register for command 4.
 *
 * The ESP I2C bus is shared hardware, so those operations must not overlap.
 * This small wrapper owns a mutex and serializes the high-level camera calls.
 */
void mlx90640_runtime_init(void);

/*
 * Read one MLX90640 subpage while holding the camera mutex.
 */
int mlx90640_runtime_get_frame_data(
    uint8_t slave_addr,
    uint16_t *frame_data
);

/*
 * Change the MLX90640 refresh-rate register while holding the camera mutex.
 */
int mlx90640_runtime_set_refresh_rate(
    uint8_t slave_addr,
    uint8_t refresh_rate_code
);
