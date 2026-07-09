#include "MLX90640_I2C_Driver.h"

#include <stdint.h>
#include <stdlib.h>

#include "driver/i2c.h"
#include "esp_err.h"

#include "camera_i2c.h"   // must expose I2C_PORT

#define MLX90640_I2C_OK 0
#define MLX90640_I2C_ERROR -1

#define MLX90640_READ_CHUNK_WORDS 16

int MLX90640_I2CRead(
    uint8_t slaveAddr,
    uint16_t startAddress,
    uint16_t nMemAddressRead,
    uint16_t *data
)
{
    static uint8_t rx[MLX90640_READ_CHUNK_WORDS * 2];

    uint16_t words_read = 0;

    while (words_read < nMemAddressRead) {
        uint16_t words_this_time = nMemAddressRead - words_read;

        if (words_this_time > MLX90640_READ_CHUNK_WORDS) {
            words_this_time = MLX90640_READ_CHUNK_WORDS;
        }

        uint16_t addr = startAddress + words_read;

        uint8_t reg_addr[2] = {
            (uint8_t)(addr >> 8),
            (uint8_t)(addr & 0xFF),
        };

        esp_err_t ret = i2c_master_write_read_device(
            I2C_PORT,
            slaveAddr,
            reg_addr,
            2,
            rx,
            words_this_time * 2,
            pdMS_TO_TICKS(1000)
        );

        if (ret != ESP_OK) {
            printf("I2CRead failed at 0x%04X words=%u: %s (%d)\n",
                   addr,
                   words_this_time,
                   esp_err_to_name(ret),
                   ret);
            return -1;
        }

        for (uint16_t i = 0; i < words_this_time; i++) {
            data[words_read + i] =
                ((uint16_t)rx[2 * i] << 8) |
                rx[2 * i + 1];
        }

        words_read += words_this_time;
    }

    return 0;
}

int MLX90640_I2CWrite(
    uint8_t slaveAddr,
    uint16_t writeAddress,
    uint16_t data
)
{
    uint8_t tx[4] = {
        (uint8_t)(writeAddress >> 8),
        (uint8_t)(writeAddress & 0xFF),
        (uint8_t)(data >> 8),
        (uint8_t)(data & 0xFF),
    };

    esp_err_t ret = i2c_master_write_to_device(
        I2C_PORT,
        slaveAddr,
        tx,
        sizeof(tx),
        pdMS_TO_TICKS(100)
    );

    if (ret != ESP_OK) {
        return MLX90640_I2C_ERROR;
    }

    return MLX90640_I2C_OK;
}
