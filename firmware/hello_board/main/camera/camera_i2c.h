#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "protocol/thermal_packet.h"

#define MLX90640_EEPROM_START 0x2440
#define MLX90640_EEPROM_END   0x273F
#define MLX90640_EEPROM_WORDS \
    (MLX90640_EEPROM_END - MLX90640_EEPROM_START + 1)

#define MLX90640_RAM_START 0x0400
#define MLX90640_FRAME_WORDS 834


#define I2C_PORT        I2C_NUM_0


void camera_i2c_init(void);
void camera_i2c_scan(void);

esp_err_t mlx90640_read_register(uint16_t , uint16_t *value);
esp_err_t mlx90640_read_eeprom(uint16_t eeprom[]);
esp_err_t mlx90640_read_frame_words(uint16_t frame[]);
void raw_frame_to_pixels(
    const uint16_t frame_words[THERMAL_PIXELS],
    uint8_t pixels[THERMAL_PIXELS]
);
