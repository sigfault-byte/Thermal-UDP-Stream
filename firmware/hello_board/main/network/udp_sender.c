#include "udp_sender.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "network/stream_control.h"
#include "protocol/thermal_packet.h"
#include "camera/camera_i2c.h"
#include "camera/MLX90640_API.h"
#include "utils/quantization.h"

#define DEST_PORT 5005

static const char *TAG = "udp_sender";

void udp_sender_task(void *pvParameters)
{
    paramsMLX90640 *params = (paramsMLX90640 *)pvParameters;

    uint32_t counter = 0;

    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DEST_PORT),
    };

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket");
        vTaskDelete(NULL);
        return;
    }

    static uint16_t frameData[834];
    static float temp_subpage[THERMAL_PIXELS];
    static float temperatures[THERMAL_PIXELS];
    static uint8_t pixels[THERMAL_PIXELS];
    static thermal_packet_t packet;

    while (1) {
        uint32_t destination_addr = 0;

        ESP_LOGI(TAG, "Waiting for START command before reading camera frames");

        /*
         * Sleep here while stopped. This keeps the task alive without reading
         * MLX90640 frames or burning CPU before the viewer asks for streaming.
         */
        stream_control_wait_started();

        /*
         * START stores the destination selected by the TCP command server.
         * If STOP raced with this wake-up, loop back and wait again.
         */
        if (!stream_control_get_destination(&destination_addr)) {
            continue;
        }

        while (stream_control_is_running()) {
            int have0 = 0;
            int have1 = 0;

            while (stream_control_is_running() && (!have0 || !have1)) {
                int subpage = MLX90640_GetFrameData(0x33, frameData);
                if (subpage < 0) {
                    ESP_LOGW(TAG, "Failed to read MLX90640 frame: %d", subpage);
                    continue;
                }

                float ta = MLX90640_GetTa(frameData, params);
                float tr = ta - 8.0f;

                MLX90640_CalculateTo(
                    frameData,
                    params,
                    0.95f,
                    tr,
                    temp_subpage
                );

                for (int row = 0; row < THERMAL_HEIGHT; row++) {
                    for (int column = 0; column < THERMAL_WIDTH; column++) {
                        const int index =
                            row * THERMAL_WIDTH + column;

                        /*
                         * MLX90640 subpages are interleaved like a checkerboard.
                         *
                         * (row + column) & 1 checks the last bit:
                         *
                         *   0 -> even checkerboard square
                         *   1 -> odd checkerboard square
                         *
                         * We only copy the pixels that belong to the subpage
                         * returned by MLX90640_GetFrameData().
                         */
                        const int pixel_subpage =
                            (row + column) & 1;

                        if (pixel_subpage == subpage) {
                            temperatures[index] =
                                temp_subpage[index];
                        }
                    }
                }

                // for (int i = 0; i < THERMAL_PIXELS; i++) {
                //     if (temp_subpage[i] != 0.0f) {
                //         temperatures[i] = temp_subpage[i];
                //     }
                // }

                if (subpage == 0) have0 = 1;
                if (subpage == 1) have1 = 1;
            }

            if (!stream_control_is_running()) {
                ESP_LOGI(TAG, "Streaming stopped before a complete frame was ready");
                break;
            }

            temps_to_pixels(temperatures, pixels);

            thermal_packet_init(
                &packet,
                counter,
                (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS),
                pixels
            );

            /*
             * Re-read the destination before sending so repeated START can
             * retarget an already-running stream to the newest TCP peer IP.
             */
            if (!stream_control_get_destination(&destination_addr)) {
                ESP_LOGI(TAG, "Streaming stopped before send");
                break;
            }

            dest_addr.sin_addr.s_addr = destination_addr;

            sendto(sock, &packet, sizeof(packet), 0,
                   (struct sockaddr *)&dest_addr,
                   sizeof(dest_addr));

            counter++;
            // vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
}
