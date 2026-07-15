#include "camera_i2c.h"
#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "protocol/thermal_packet.h"

#define I2C_SDA_GPIO    33
#define I2C_SCL_GPIO    14
#define I2C_PULL_ENABLE_GPIO 3

#define I2C_FREQ_HZ     400000

void raw_frame_to_pixels(
    const uint16_t frame_words[THERMAL_PIXELS],
    uint8_t pixels[THERMAL_PIXELS]
)
{
    int16_t min = INT16_MAX;
    int16_t max = INT16_MIN;

    for (int i = 0; i < THERMAL_PIXELS; i++) {
        int16_t v = (int16_t)frame_words[i];

        if (v < min) min = v;
        if (v > max) max = v;
    }

    int range = max - min;

    if (range == 0) {
        memset(pixels, 0, THERMAL_PIXELS);
        return;
    }

    for (int i = 0; i < THERMAL_PIXELS; i++) {
        int16_t v = (int16_t)frame_words[i];

        pixels[i] = (uint8_t)(((int)(v - min) * 254) / range);
    }
}

void camera_i2c_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << I2C_PULL_ENABLE_GPIO);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(I2C_PULL_ENABLE_GPIO, 0);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    esp_err_t ret;

    ret = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    printf("i2c_driver_install: %s (%d)\n", esp_err_to_name(ret), ret);
    ESP_ERROR_CHECK(ret);

    ret = i2c_param_config(I2C_PORT, &conf);
    printf("i2c_param_config: %s (%d)\n", esp_err_to_name(ret), ret);
    ESP_ERROR_CHECK(ret);


}

void camera_i2c_scan(void)
{
    printf("Scanning I2C bus...\n");

    printf("I2C using SDA=%d SCL=%d FREQ=%d\n",
           I2C_SDA_GPIO,
           I2C_SCL_GPIO,
           I2C_FREQ_HZ);

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();

        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(
            I2C_PORT,
            cmd,
            pdMS_TO_TICKS(50)
        );

        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            printf("0x%02X -> ACK / DEVICE FOUND\n", addr);
        }
        // else {
        //     printf("0x%02X -> %s (%d)\n",
        //            addr,
        //            esp_err_to_name(ret),
        //            ret);
        // }

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    printf("I2C scan done\n");
}

esp_err_t mlx90640_read_register(uint16_t reg, uint16_t *value)
{

    uint8_t reg_addr[2] = {
        (uint8_t)(reg >> 8),
        (uint8_t)(reg & 0xFF),
    };

    uint8_t data[2];

    esp_err_t ret = i2c_master_write_read_device(
        I2C_PORT,
        0x33,
        reg_addr,
        2,
        data,
        2,
        pdMS_TO_TICKS(100)
    );

    if (ret != ESP_OK) {
        return ret;
    }

    *value = ((uint16_t)data[0] << 8) | data[1];

    return ESP_OK;
}


esp_err_t mlx90640_read_eeprom(uint16_t eeprom[])
{
    for (uint16_t addr = MLX90640_EEPROM_START;
         addr <= MLX90640_EEPROM_END;
         addr++)
    {
        uint16_t word;

        esp_err_t ret =
            mlx90640_read_register(addr, &word);

        if (ret != ESP_OK) {
            return ret;
        }

        eeprom[addr - MLX90640_EEPROM_START] = word;
    }

    return ESP_OK;
}

esp_err_t mlx90640_read_frame_words(uint16_t frame[])
{
    for (uint16_t i = 0; i < MLX90640_FRAME_WORDS; i++) {
        esp_err_t ret = mlx90640_read_register(
            MLX90640_RAM_START + i,
            &frame[i]
        );

        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}
