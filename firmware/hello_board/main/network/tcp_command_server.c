#include "tcp_command_server.h"

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "network/stream_control.h"
#include "protocol/command_packet.h"

/*
 * This is the command TCP port.
 * UDP port 5005 continues to carry thermal frames.
 */
#define TCP_COMMAND_PORT 5006

/*
 * This server handles one command client at a time.
 * A backlog of one keeps one pending connection waiting while the current
 * client is being served.
 */
#define TCP_COMMAND_BACKLOG 1

#define TCP_COMMAND_RX_BUFFER_SIZE 64

/*
 * TCP is a byte stream, so recv() can return a partial command packet, one
 * complete command packet, or several command packets together.
 */
#define TCP_COMMAND_ACCUMULATOR_SIZE \
    (TCP_COMMAND_RX_BUFFER_SIZE * 2)

/*
 * If creating, binding, listening, or accepting fails, the task waits before
 * trying again so it does not spin and starve other FreeRTOS tasks.
 */
#define TCP_COMMAND_RETRY_DELAY_MS 1000

static const char *TAG = "tcp_command";

/*
 * Turn bytes into a compact hex preview.
 * This remains useful when a malformed command packet arrives.
 */
static void log_received_bytes(
    const uint8_t *buffer,
    int length
)
{
    /* Each byte needs two hex digits plus one separator. */
    char hex_preview[(TCP_COMMAND_RX_BUFFER_SIZE * 3) + 1];

    /* offset tracks the next free write position in hex_preview. */
    int offset = 0;

    /* Convert each received byte into text for the ESP-IDF log. */
    for (int i = 0; i < length; i++) {
        /* Append "AA" for the last byte or "AA " between bytes. */
        offset += snprintf(
            hex_preview + offset,
            sizeof(hex_preview) - offset,
            "%02X%s",
            buffer[i],
            i == length - 1 ? "" : " "
        );

        /* Stop if the preview buffer is full. */
        if (offset >= (int)sizeof(hex_preview)) {
            break;
        }
    }

    /* Log the raw byte count and the hex preview for host-side debugging. */
    ESP_LOGI(TAG, "Received %d byte(s): %s", length, hex_preview);
}

static bool send_command_response(
    int client_sock,
    uint8_t command,
    uint8_t status
)
{
    command_response_t response;

    /*
     * The protocol helper fills the shared magic/version and the TCP server
     * chooses which command/status should be echoed back.
     */
    command_response_init(
        &response,
        command,
        status
    );

    /*
     * The response is only 11 bytes, so one send() should normally write it
     * all. Still, check the byte count because TCP write errors matter.
     */
    ssize_t bytes_sent = send(
        client_sock,
        &response,
        sizeof(response),
        0
    );

    if (bytes_sent != (ssize_t)sizeof(response)) {
        ESP_LOGE(
            TAG,
            "Failed to send command response, sent=%zd errno=%d",
            bytes_sent,
            errno
        );
        return false;
    }

    ESP_LOGI(
        TAG,
        "Command response sent: command=%u status=%u",
        command,
        status
    );

    return true;
}

static bool process_command_packet(
    int client_sock,
    uint32_t peer_addr_s_addr,
    const uint8_t packet_bytes[COMMAND_PACKET_SIZE]
)
{
    command_packet_t packet;
    uint8_t status = COMMAND_STATUS_OK;

    /*
     * Copy raw TCP bytes into the packed request structure.
     * This avoids alignment surprises from casting a byte buffer directly.
     */
    memcpy(
        &packet,
        packet_bytes,
        sizeof(packet)
    );

    /*
     * If the request is malformed, echo the command byte when possible so the
     * viewer can correlate the response with its pending command.
     */
    if (!command_packet_is_valid_request(&packet)) {
        status = COMMAND_STATUS_ERROR;

        ESP_LOGW(TAG, "Invalid command packet");
        log_received_bytes(packet_bytes, COMMAND_PACKET_SIZE);
    } else {
        /*
         * START uses the TCP peer IP as the UDP thermal frame destination.
         * The viewer discovers the ESP, connects back over TCP, and that TCP
         * peer address becomes the unicast destination for UDP frames.
         */
        if (packet.command == COMMAND_START) {
            stream_control_start(peer_addr_s_addr);
        }

        /*
         * STOP disables frame reads/sends but leaves discovery and TCP alive.
         */
        if (packet.command == COMMAND_STOP) {
            stream_control_stop();
        }

        ESP_LOGI(
            TAG,
            "Command received: command=%u value=%u",
            packet.command,
            packet.value
        );
    }

    return send_command_response(
        client_sock,
        packet.command,
        status
    );
}

/*
 * Serve one connected client until it disconnects or the socket reports an
 * error. This keeps connection handling separate from the listening loop.
 */
static void handle_client(
    int client_sock,
    const struct sockaddr_in *client_addr
)
{
    /* Holds the printable IPv4 address of the connected client. */
    char client_ip[INET_ADDRSTRLEN];

    /* Holds one chunk of raw bytes returned by recv(). */
    uint8_t rx_buffer[TCP_COMMAND_RX_BUFFER_SIZE];

    /*
     * Holds bytes across recv() calls until at least one full 11-byte command
     * packet is available.
     */
    uint8_t command_buffer[TCP_COMMAND_ACCUMULATOR_SIZE];

    /* Number of bytes currently waiting in command_buffer. */
    size_t command_buffer_length = 0;

    /* Set false when a read/write failure means this client should close. */
    bool client_alive = true;

    /*
     * Capture the peer IP once. START commands from this TCP client use this
     * address as the UDP thermal destination.
     */
    uint32_t peer_addr_s_addr = client_addr->sin_addr.s_addr;

    /* Convert the client's binary IPv4 address into dotted decimal text. */
    inet_ntop(
        AF_INET,
        &client_addr->sin_addr,
        client_ip,
        sizeof(client_ip)
    );

    /* Log the peer address so a test computer can be matched to the ESP log. */
    ESP_LOGI(
        TAG,
        "Client connected from %s:%u",
        client_ip,
        ntohs(client_addr->sin_port)
    );

    /* Read from the connected TCP stream until the client goes away. */
    while (client_alive) {
        /* recv blocks this task until data, disconnect, or an error arrives. */
        int bytes_received = recv(
            client_sock,
            rx_buffer,
            sizeof(rx_buffer),
            0
        );

        /* A positive return means bytes were received into rx_buffer. */
        if (bytes_received > 0) {
            /*
             * If the accumulator would overflow, drop the partial bytes and
             * start fresh. This is simpler than pretending v1 can resync an
             * arbitrary corrupted TCP stream.
             */
            if (
                command_buffer_length + bytes_received
                > sizeof(command_buffer)
            ) {
                ESP_LOGW(TAG, "Command buffer overflow, dropping partial data");
                command_buffer_length = 0;
            }

            /*
             * Append this recv() chunk to the accumulator. TCP may split or
             * merge command packets however it wants.
             */
            memcpy(
                command_buffer + command_buffer_length,
                rx_buffer,
                bytes_received
            );
            command_buffer_length += bytes_received;

            /*
             * Process every complete 11-byte packet currently buffered.
             * Partial trailing bytes stay buffered for the next recv().
             */
            while (command_buffer_length >= COMMAND_PACKET_SIZE) {
                if (
                    !process_command_packet(
                        client_sock,
                        peer_addr_s_addr,
                        command_buffer
                    )
                ) {
                    client_alive = false;
                    break;
                }

                command_buffer_length -= COMMAND_PACKET_SIZE;

                memmove(
                    command_buffer,
                    command_buffer + COMMAND_PACKET_SIZE,
                    command_buffer_length
                );
            }

            continue;
        }

        /* A zero return means the client closed the connection normally. */
        if (bytes_received == 0) {
            ESP_LOGI(TAG, "Client disconnected");
            break;
        }

        /* A negative return means the socket failed; errno explains why. */
        ESP_LOGE(TAG, "recv failed, errno=%d", errno);
        break;
    }

    /* Tell the TCP stack we are done sending and receiving on this client. */
    shutdown(client_sock, SHUT_RDWR);

    /* Release the client socket descriptor. */
    close(client_sock);
}

void tcp_command_server_task(void *pvParameters)
{
    /* This task does not need startup parameters yet. */
    (void)pvParameters;

    /* Keep the server alive for the lifetime of the firmware. */
    while (1) {
        /*
         * Bind to all local IPv4 addresses on port 5006.
         * INADDR_ANY is correct for station mode because the DHCP address is
         * assigned after Wi-Fi connects.
         */
        struct sockaddr_in listen_addr = {
            .sin_addr.s_addr = htonl(INADDR_ANY),
            .sin_family = AF_INET,
            .sin_port = htons(TCP_COMMAND_PORT),
        };

        /* Create an IPv4 TCP socket for accepting command connections. */
        int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

        /* If socket creation fails, wait and retry instead of deleting task. */
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket, errno=%d", errno);
            vTaskDelay(pdMS_TO_TICKS(TCP_COMMAND_RETRY_DELAY_MS));
            continue;
        }

        /*
         * Allow the port to be reused after a quick restart.
         * If the option is unsupported, bind may still succeed, so v1 does not
         * treat setsockopt failure as fatal.
         */
        int reuse_addr = 1;
        setsockopt(
            listen_sock,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse_addr,
            sizeof(reuse_addr)
        );

        /* Attach the listening socket to local TCP port 5006. */
        int err = bind(
            listen_sock,
            (struct sockaddr *)&listen_addr,
            sizeof(listen_addr)
        );

        /* If bind fails, close this socket and retry from a clean descriptor. */
        if (err != 0) {
            ESP_LOGE(TAG, "Socket bind failed, errno=%d", errno);
            close(listen_sock);
            vTaskDelay(pdMS_TO_TICKS(TCP_COMMAND_RETRY_DELAY_MS));
            continue;
        }

        /* Mark the socket as passive so accept can receive TCP clients. */
        err = listen(listen_sock, TCP_COMMAND_BACKLOG);

        /* If listen fails, close this socket and retry after a short delay. */
        if (err != 0) {
            ESP_LOGE(TAG, "Socket listen failed, errno=%d", errno);
            close(listen_sock);
            vTaskDelay(pdMS_TO_TICKS(TCP_COMMAND_RETRY_DELAY_MS));
            continue;
        }

        /* This log is the host-side signal that nc can connect to port 5006. */
        ESP_LOGI(TAG, "Listening for TCP commands on port %d", TCP_COMMAND_PORT);

        /* Accept and serve one client at a time on this listening socket. */
        while (1) {
            /* accept fills this structure with the remote client address. */
            struct sockaddr_in client_addr;

            /* accept needs the size of client_addr and may update it. */
            socklen_t client_addr_len = sizeof(client_addr);

            /* Block until a TCP client connects or the listening socket fails. */
            int client_sock = accept(
                listen_sock,
                (struct sockaddr *)&client_addr,
                &client_addr_len
            );

            /* On accept failure, rebuild the listening socket from scratch. */
            if (client_sock < 0) {
                ESP_LOGE(TAG, "Socket accept failed, errno=%d", errno);
                break;
            }

            /* Handle this client fully before accepting the next one. */
            handle_client(client_sock, &client_addr);
        }

        /* Release the failed listening socket before the outer retry loop. */
        close(listen_sock);

        /* Avoid a tight failure loop if the TCP stack is temporarily unhappy. */
        vTaskDelay(pdMS_TO_TICKS(TCP_COMMAND_RETRY_DELAY_MS));
    }
}
