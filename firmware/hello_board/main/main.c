#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led/led.h"
#include "wifi/wifi_sta.h"
#include "network/udp_sender.h"
#include "camera/camera_i2c.h"
#include "camera/MLX90640_I2C_Driver.h"
#include "camera/MLX90640_API.h"

#define MLX90640_ADDR 0x33
#define MLX90640_CONTROL_REG 0x800D
#define UDP_TASK_STACK_SIZE 8192
#define UDP_TASK_PRIORITY 5

static uint16_t ee_data[MLX90640_EEPROM_WORDS];
static paramsMLX90640 mlx_params;

static void startup_led_test(void)
{
    printf("LED init\n");
    led_init();

    printf("Red LED\n");
    led_set(255, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(250));

    printf("Green LED\n");
    led_set(0, 255, 0);
    vTaskDelay(pdMS_TO_TICKS(250));

    printf("Blue LED\n");
    led_set(0, 0, 255);
    vTaskDelay(pdMS_TO_TICKS(250));

    led_set(0, 0, 0);
    printf("LED test OK\n");
}

static bool mlx90640_probe(void)
{
    uint16_t control = 0;

    int err = MLX90640_I2CRead(
        MLX90640_ADDR,
        MLX90640_CONTROL_REG,
        1,
        &control
    );

    if (err != MLX90640_NO_ERROR) {
        printf("MLX90640 probe failed: %d\n", err);
        return false;
    }

    printf("MLX90640 control register 0x%04X = 0x%04X\n",
           MLX90640_CONTROL_REG,
           control);

    return true;
}

static bool mlx90640_load_calibration(void)
{
    int err = MLX90640_DumpEE(MLX90640_ADDR, ee_data);

    if (err != MLX90640_NO_ERROR) {
        printf("MLX90640_DumpEE failed: %d\n", err);
        return false;
    }

    printf("MLX90640_DumpEE OK\n");

    err = MLX90640_ExtractParameters(ee_data, &mlx_params);

    if (err != MLX90640_NO_ERROR) {
        printf("MLX90640_ExtractParameters failed: %d\n", err);
        return false;
    }

    printf("MLX90640_ExtractParameters OK\n");
    return true;
}

void app_main(void)
{
    printf("Hello from ESP32\n");

    startup_led_test();

    printf("I2C init\n");
    camera_i2c_init();

    printf("I2C scan\n");
    camera_i2c_scan();

    if (!mlx90640_probe()) {
        printf("Stopping: MLX90640 probe failed\n");
        return;
    }

    if (!mlx90640_load_calibration()) {
        printf("Stopping: MLX90640 calibration load failed\n");
        return;
    }

    printf("Wi-Fi init\n");
    wifi_init_sta();
    wifi_wait_connected();

    printf("Starting UDP thermal stream\n");

    xTaskCreate(
        udp_sender_task,
        "udp_sender",
        UDP_TASK_STACK_SIZE,
        &mlx_params,
        UDP_TASK_PRIORITY,
        NULL
    );
}
