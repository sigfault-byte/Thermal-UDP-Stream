#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led/led.h"
#include "wifi/wifi_sta.h"
#include "network/udp_sender.h"
#include "network/tcp_command_server.h"
#include "network/handshake_broadcaster.h"
#include "network/stream_control.h"
#include "camera/camera_i2c.h"
#include "camera/MLX90640_I2C_Driver.h"
#include "camera/MLX90640_API.h"
#include "camera/mlx90640_runtime.h"
#include "utils/refresh_rate.h"

#define MLX90640_ADDR 0x33
#define MLX90640_CONTROL_REG 0x800D
#define UDP_TASK_STACK_SIZE 8192
#define UDP_TASK_PRIORITY 5
#define TCP_COMMAND_TASK_STACK_SIZE 4096
#define TCP_COMMAND_TASK_PRIORITY 5
#define HANDSHAKE_TASK_STACK_SIZE 4096
#define HANDSHAKE_TASK_PRIORITY 5

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

static bool mlx90640_set_default_refresh_rate(void)
{
    /*
     * Boot default is intentionally slow and friendly: 1 Hz.
     *
     * The TCP SET_REFRESH_RATE command can later switch this to 8 Hz.
     * The MLX90640 API expects the compact register code, not the readable
     * value used by the TCP packet.
     */
    int err = mlx90640_runtime_set_refresh_rate(
        MLX90640_ADDR,
        refresh_rate_to_mlx90640_code(REFRESH_RATE_1_HZ)
    );

    if (err != MLX90640_NO_ERROR) {
        printf("MLX90640_SetRefreshRate default failed: %d\n", err);
        return false;
    }

    printf("MLX90640 refresh default set to %u Hz\n", REFRESH_RATE_1_HZ);
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

    printf("MLX90640 runtime init\n");
    mlx90640_runtime_init();

    if (!mlx90640_set_default_refresh_rate()) {
        printf("Stopping: MLX90640 default refresh failed\n");
        return;
    }

    printf("Wi-Fi init\n");
    wifi_init_sta();
    wifi_wait_connected();

    printf("Stream control init\n");
    stream_control_init();

    printf("Starting handshake broadcaster\n");

    /*
     * Start periodic UDP broadcasts after Wi-Fi has an IP address.
     * Receivers use these packets for discovery, then connect over TCP 5006.
     */
    xTaskCreate(
        handshake_broadcaster_task,  // FreeRTOS task function
        "handshake",                // Task name shown in diagnostics
        HANDSHAKE_TASK_STACK_SIZE,  // Stack reserved for UDP broadcast work
        NULL,                       // No startup parameter is needed yet
        HANDSHAKE_TASK_PRIORITY,    // Same priority class as network tasks
        NULL                        // No task handle is needed by app_main
    );

    printf("Starting TCP command server\n");

    /*
     * Start the TCP command server after Wi-Fi has an IP address.
     * The task owns the listening socket and handles one client at a time.
     */
    xTaskCreate(
        tcp_command_server_task,       // FreeRTOS task function
        "tcp_command",                // Task name shown in diagnostics
        TCP_COMMAND_TASK_STACK_SIZE,  // Stack reserved for socket handling
        NULL,                         // No startup parameter is needed yet
        TCP_COMMAND_TASK_PRIORITY,    // Same priority class as UDP streaming
        NULL                          // No task handle is needed by app_main
    );

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
