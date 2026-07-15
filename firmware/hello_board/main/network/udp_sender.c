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
#include "camera/mlx90640_runtime.h"
#include "utils/quantization.h"

#include "esp_timer.h"
#include <inttypes.h>

#define DEST_PORT 5005

static const char *TAG = "udp_sender";

void udp_sender_task(void *pvParameters)
{
    paramsMLX90640 *params = (paramsMLX90640 *)pvParameters;

    uint32_t counter = 0;
    int64_t acquisition_sequence = 0;
    int64_t previous_send_us = 0;

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
        previous_send_us = 0;

        while (stream_control_is_running()) {
            int complete_pair = 0;
            int pending_subpage = -1;
            int64_t pending_sequence = 0;
            int64_t pending_acquired_us = 0;

            while (stream_control_is_running() && !complete_pair) {
                const int64_t read_start_us =
                    esp_timer_get_time();

                int subpage = mlx90640_runtime_get_frame_data(0x33, frameData);

                const int64_t read_end_us =
                    esp_timer_get_time();

                if (subpage < 0) {
                    ESP_LOGW(TAG, "Failed to read MLX90640 frame: %d", subpage);
                    continue;
                }

                if (subpage != 0 && subpage != 1) {
                    ESP_LOGW(
                        TAG,
                        "Ignoring invalid MLX90640 subpage type %d",
                        subpage
                    );
                    continue;
                }

                acquisition_sequence++;
                const int64_t current_sequence = acquisition_sequence;
                const int64_t current_acquired_us = read_end_us;
                const int replaces_pending =
                    pending_subpage == subpage;

                if (replaces_pending) {
                    ESP_LOGW(
                        TAG,
                        "Replacing stale duplicate subpage %d: "
                        "old_seq=%" PRId64 " new_seq=%" PRId64,
                        subpage,
                        pending_sequence,
                        current_sequence
                    );
                }

                const float ta = MLX90640_GetTa(frameData, params);
                const float tr = ta - 8.0f;

                const int64_t calculate_start_us =
                    esp_timer_get_time();

                MLX90640_CalculateTo(
                    frameData,
                    params,
                    0.95f,
                    tr,
                    temp_subpage
                );

                const int64_t calculate_end_us =
                    esp_timer_get_time();

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

                /*
                 * Keep exactly one pending half. Same-type acquisitions replace
                 * stale pixels; opposite-type acquisitions complete the frame.
                 */
                if (pending_subpage < 0 || replaces_pending) {
                    pending_subpage = subpage;
                    pending_sequence = current_sequence;
                    pending_acquired_us = current_acquired_us;
                } else {
                    complete_pair = 1;

                    ESP_LOGI(
                        TAG,
                        "Complete subpage pair: pending=%d seq=%" PRId64
                        " age=%" PRId64 " us current=%d seq=%" PRId64,
                        pending_subpage,
                        pending_sequence,
                        current_acquired_us - pending_acquired_us,
                        subpage,
                        current_sequence
                    );
                }

                ESP_LOGI(
                    TAG,
                    "subpage=%d seq=%" PRId64
                    " read=%" PRId64 " us calculate=%" PRId64 " us",
                    subpage,
                    current_sequence,
                    read_end_us - read_start_us,
                    calculate_end_us - calculate_start_us
                );
            }

            if (!stream_control_is_running()) {
                ESP_LOGI(TAG, "Streaming stopped before a complete frame was ready");
                break;
            }

            /*
             * Read the current quantization mode after the complete float frame
             * is ready. A mode command received during this frame will apply
             * here or on the next frame, depending on exact timing.
             */
            uint8_t quantization_mode =
                stream_control_get_quantization_mode();

            temps_to_pixels(
                temperatures,
                quantization_mode,
                pixels
            );

            thermal_packet_init(
                &packet,
                counter,
                (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS),
                quantization_mode,
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

            const ssize_t sent_bytes =
                sendto(
                    sock,
                    &packet,
                    sizeof(packet),
                    0,
                    (struct sockaddr *)&dest_addr,
                    sizeof(dest_addr)
                );

            if (sent_bytes < 0) {
                ESP_LOGE(
                    TAG,
                    "Failed to send thermal packet"
                );
                continue;
            }

            if ((size_t)sent_bytes != sizeof(packet)) {
                ESP_LOGW(
                    TAG,
                    "Partial UDP send: sent=%d expected=%d",
                    (int)sent_bytes,
                    (int)sizeof(packet)
                );
                continue;
            }

            const int64_t now_us =
                esp_timer_get_time();

            if (previous_send_us != 0) {
                const int64_t interval_us =
                    now_us - previous_send_us;

                ESP_LOGI(
                    TAG,
                    "Complete frame interval: %" PRId64
                    " us, FPS: %.2f",
                    interval_us,
                    1000000.0 / (double)interval_us
                );
            }

            previous_send_us = now_us;
            counter++;
        }
    }
}
