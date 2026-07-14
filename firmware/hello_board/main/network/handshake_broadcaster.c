#include "handshake_broadcaster.h"

#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "protocol/handshake_packet.h"

#define HANDSHAKE_BROADCAST_IP "192.168.1.255"
#define HANDSHAKE_BROADCAST_PORT 5004
#define HANDSHAKE_BROADCAST_INTERVAL_MS 5000
#define HANDSHAKE_RETRY_DELAY_MS 1000

static const char *TAG = "handshake_broadcast";

void handshake_broadcaster_task(void *pvParameters)
{
    /* This task does not need startup parameters yet. */
    (void)pvParameters;

    /*
     * The task is long-lived. If a socket setup error happens, the outer loop
     * closes that socket and tries again after a short delay.
     */
    while (1) {
        /*
         * UDP broadcast is still UDP: the destination address selects the LAN
         * broadcast range, and the destination port selects the receiver socket.
         */
        struct sockaddr_in broadcast_addr = {
            .sin_addr.s_addr = inet_addr(HANDSHAKE_BROADCAST_IP),
            .sin_family = AF_INET,
            .sin_port = htons(HANDSHAKE_BROADCAST_PORT),
        };

        /* Create an IPv4 UDP socket for sending discovery announcements. */
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

        /* If socket creation fails, wait and retry without killing firmware. */
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket, errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(HANDSHAKE_RETRY_DELAY_MS));
            continue;
        }

        /*
         * Allow this UDP socket to send to 192.168.1.255.
         * Without SO_BROADCAST, the stack may reject broadcast sendto calls.
         */
        int broadcast_enabled = 1;
        int err = setsockopt(
            sock,
            SOL_SOCKET,
            SO_BROADCAST,
            &broadcast_enabled,
            sizeof(broadcast_enabled)
        );

        /* A failure here means broadcast sends are unlikely to work. */
        if (err != 0) {
            ESP_LOGE(TAG, "Unable to enable broadcast, errno=%d", errno);
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(HANDSHAKE_RETRY_DELAY_MS));
            continue;
        }

        ESP_LOGI(
            TAG,
            "Broadcasting handshake to %s:%d every %d ms",
            HANDSHAKE_BROADCAST_IP,
            HANDSHAKE_BROADCAST_PORT,
            HANDSHAKE_BROADCAST_INTERVAL_MS
        );

        /* Keep sending a fresh obfuscated packet on the working socket. */
        while (1) {
            handshake_packet_t packet;

            /*
             * Rebuild the cleartext packet before every send.
             * This keeps the encode step obviously one-way for the outgoing
             * buffer and avoids accidentally XORing an already encoded packet.
             */
            handshake_packet_init(&packet);

            /*
             * Apply the repeating-key XOR immediately before sendto so the
             * bytes on the wire are not the cleartext PADAWAN marker.
             */
            handshake_packet_encode(&packet);

            int bytes_sent = sendto(
                sock,
                &packet,
                sizeof(packet),
                0,
                (struct sockaddr *)&broadcast_addr,
                sizeof(broadcast_addr)
            );

            if (bytes_sent < 0) {
                ESP_LOGE(TAG, "Handshake broadcast failed, errno=%d", errno);
                break;
            }

            ESP_LOGI(TAG, "Sent %d-byte handshake broadcast", bytes_sent);

            vTaskDelay(pdMS_TO_TICKS(HANDSHAKE_BROADCAST_INTERVAL_MS));
        }

        /* Release the failed socket before trying to create a fresh one. */
        close(sock);

        /* Avoid a tight retry loop if Wi-Fi or the UDP stack is unhappy. */
        vTaskDelay(pdMS_TO_TICKS(HANDSHAKE_RETRY_DELAY_MS));
    }
}
