#include "handshake_broadcaster.h"

#include <errno.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "protocol/handshake_packet.h"

#define HANDSHAKE_BROADCAST_PORT 5004
#define HANDSHAKE_BROADCAST_INTERVAL_MS 5000
#define HANDSHAKE_RETRY_DELAY_MS 1000

static const char *TAG = "handshake_broadcast";

/*
 * Build the IPv4 broadcast destination from the address DHCP gave the station.
 *
 * This avoids assuming the LAN is always 192.168.1.0/24. If the AP gives the
 * ESP32 another subnet, discovery should follow that subnet automatically.
 */
static bool get_sta_broadcast_addr(struct sockaddr_in *broadcast_addr)
{
    /*
     * esp_netif_create_default_wifi_sta() registers the station netif with
     * this default key. The broadcaster starts after GOT_IP, so it should
     * already exist here.
     */
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Unable to find Wi-Fi STA netif");
        return false;
    }

    /*
     * Read the current IP, netmask, and gateway from the station interface.
     * We only need IP and netmask to calculate the subnet broadcast address.
     */
    esp_netif_ip_info_t ip_info;
    esp_err_t err = esp_netif_get_ip_info(sta_netif, &ip_info);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to get STA IP info, err=0x%x", err);
        return false;
    }

    if (ip_info.ip.addr == 0 || ip_info.netmask.addr == 0) {
        ESP_LOGE(TAG, "STA IP info is not ready");
        return false;
    }

    /*
     * ip_info values are stored in network byte order. Convert to host order
     * for the bit operation, then convert the broadcast address back before
     * putting it into sockaddr_in.
     */
    const uint32_t ip = ntohl(ip_info.ip.addr);
    const uint32_t netmask = ntohl(ip_info.netmask.addr);

    /*
     * Directed broadcast formula:
     *
     *   broadcast = ip | inverse(netmask)
     *
     * Example: 192.168.1.9/255.255.255.0 -> 192.168.1.255.
     */
    const uint32_t broadcast = ip | ~netmask;

    broadcast_addr->sin_addr.s_addr = htonl(broadcast);
    broadcast_addr->sin_family = AF_INET;
    broadcast_addr->sin_port = htons(HANDSHAKE_BROADCAST_PORT);

    esp_ip4_addr_t broadcast_ip = {
        .addr = broadcast_addr->sin_addr.s_addr,
    };

    ESP_LOGI(
        TAG,
        "STA IP " IPSTR ", netmask " IPSTR ", broadcast " IPSTR,
        IP2STR(&ip_info.ip),
        IP2STR(&ip_info.netmask),
        IP2STR(&broadcast_ip)
    );

    return true;
}

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
        struct sockaddr_in broadcast_addr = {0};

        if (!get_sta_broadcast_addr(&broadcast_addr)) {
            vTaskDelay(pdMS_TO_TICKS(HANDSHAKE_RETRY_DELAY_MS));
            continue;
        }

        /* Create an IPv4 UDP socket for sending discovery announcements. */
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

        /* If socket creation fails, wait and retry without killing firmware. */
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket, errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(HANDSHAKE_RETRY_DELAY_MS));
            continue;
        }

        /*
         * Allow this UDP socket to send to the LAN broadcast address.
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

        esp_ip4_addr_t broadcast_ip = {
            .addr = broadcast_addr.sin_addr.s_addr,
        };

        ESP_LOGI(
            TAG,
            "Broadcasting handshake to " IPSTR ":%d every %d ms",
            IP2STR(&broadcast_ip),
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
