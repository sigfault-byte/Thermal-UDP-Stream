#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Viewer-facing refresh-rate values.
 *
 * The TCP command value byte uses the human value directly:
 *
 *   1 -> request 1 Hz sensor refresh
 *   8 -> request 8 Hz sensor refresh
 *
 * The MLX90640 register itself does not store "8". It stores a compact code.
 * Keeping the mapping in this helper prevents the TCP server from carrying
 * camera-register details.
 */
#define REFRESH_RATE_1_HZ 1
#define REFRESH_RATE_8_HZ 8

/*
 * MLX90640 control-register refresh-rate codes.
 *
 * The sensor uses:
 *   code 1 -> 1 Hz
 *   code 4 -> 8 Hz
 */
#define MLX90640_REFRESH_CODE_1_HZ 1
#define MLX90640_REFRESH_CODE_8_HZ 4

/*
 * Return true only for refresh rates this firmware intentionally exposes.
 */
bool refresh_rate_is_valid(uint8_t refresh_rate_hz);

/*
 * Convert the viewer-facing value into the MLX90640 register code.
 *
 * Invalid values fall back to 1 Hz so accidental direct calls do not select a
 * surprising fast mode. Command handlers should still validate first.
 */
uint8_t refresh_rate_to_mlx90640_code(uint8_t refresh_rate_hz);
