While waiting for the male headers to arrive in order to solder them to the board and connect the thermal camera, it was a good opportunity to start implementing the Wi-Fi communication interface.

From the application's point of view, the transport mechanism changes, but the underlying idea remains remarkably similar.

```text
			Application

      serial.c          udp_sender.c
          │                  │
      stdin/stdout        socket()
          │                  │
 USB CDC driver        TCP/IP stack
          │                  │
       USB PHY          Wi-Fi driver
          │                  │
      USB cable          Radio
```
 Both interfaces ultimately move bytes between two systems. The difference lies in the protocol stack responsible for transporting those bytes.

One path relies on a point to point USB serial connection, while the other relies on the Wi-Fi stack, IP, and UDP before the payload reaches its destination.

This also illustrates one of the core ideas behind POSIX ! ! 
Files, terminals, pipes, and sockets all expose a common abstraction built around reading and writing streams of bytes.
The underlying transport may be completely different, but from the application’s perspective the operation remains conceptually the same: produce bytes, transmit them, receive bytes  interpret them.

# wifi 

The current code is

```c
static const char *TAG = "wifi_sta";

static void wifi_event_handler(
    void *arg,                    // Optional user data passed when registering handler
    esp_event_base_t event_base,   // Event category: WIFI_EVENT or IP_EVENT
    int32_t event_id,              // Specific event inside that category
    void *event_data               // Extra data attached to the event
)
{
    // The Wi-Fi station interface has started.
    // This does not mean it is connected yet.
    // It only means the Wi-Fi driver is ready to begin connecting.
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();        // Ask the driver to connect to the configured SSID
    }

    // The ESP was disconnected from the Wi-Fi network.
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // event_data is a generic void pointer.
        // For this specific event, ESP-IDF promises that it points
        // to a wifi_event_sta_disconnected_t structure.
        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;

        // Print the numeric reason code from the Wi-Fi driver.
        ESP_LOGI(TAG,
                 "Disconnected, reason = %d",
                 event->reason);

        // Try to reconnect.
        esp_wifi_connect();
    }

    // The ESP successfully got an IP address from the router.
    // This usually happens after:
    // Wi-Fi auth -> association -> DHCP -> IP assigned.
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // For this event, event_data points to an ip_event_got_ip_t.
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;

        // IPSTR and IP2STR are ESP-IDF helper macros to print IP addresses.
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}
```

Mental model:
```text
Initialize flash -> Initialize networking -> Initialize Wi-Fi -> Register callback -> Configure SSID -> Start -> Connect -> Wait for IP -> Create socket
```

```c
void wifi_init_sta(void)
{
    esp_err_t ret = nvs_flash_init(); // Wi-Fi stack stores things in NVS (Non-Volatile Storage!)

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } // repair the nvs becaue it is full / format change whatever -> basically factory reset

    ESP_ERROR_CHECK(esp_netif_init()); // init TCP/IP stack -> networking ip / routing / dhcp handshake / interfaces
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // dispatch / event queue -> calls the callback

    esp_netif_create_default_wifi_sta(); // network interface as wifi client
    // esp_netif_create_default_wifi_ap(); -> access point mode

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // init wifi drivers thanks to expressif api
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // load radio driver with the config

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL
    )); // every wifi event

    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL
    )); // every IP event

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    }; // specific credential to autheticate to the wifi access point

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // operating mode as client ( sta == client != ap = access point)
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // driver now has all the config necesary
    ESP_ERROR_CHECK(esp_wifi_start()); // drivers does what we asked for

    ESP_LOGI(TAG, "wifi_init_sta finished");
}
```

mental model
```text
Power radio -> Initialize firmware -> Generate WIFI_EVENT_STA_START -> Our callback receives it -> esp_wifi_connect() -> Authentication -> Association -> DHCP -> IP_EVENT_STA_GOT_IP -> Ready
```


# UDP Interface

```c
static const char *TAG = "udp_sender";

void udp_sender_task(void *pvParameters)
{
    // FreeRTOS task argument
    // not used atm
    (void)pvParameters;

    int counter = 0;

    // where the UDP datagrams will be sent
    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = inet_addr(DEST_IP), // Convert "192.168.1.42" to binary IP :)
        .sin_family = AF_INET,                 // AF_INET = IPv4
        .sin_port = htons(DEST_PORT),          // convert port to network byte order
    };

    // Create a UDP socket.
    //
    // AF_INET      = IPv4
    // SOCK_DGRAM   = datagram socket, so "UDP like" message packets
    // IPPROTO_IP   = let the stack choose the default protocol for SOCK_DGRAM
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

// others from sockets.h :    
    //* Socket protocol types (TCP/UDP/RAW) */
	///	#define SOCK_STREAM     1
	///	#define SOCK_DGRAM      2
	///	#define SOCK_RAW        3

    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket");
        vTaskDelete(NULL);
        return;
    } // should be a retry loop, but fine for now
    // TODO

    while (1) {
        char payload[64];

        // write formatted text into payload safely.
        // sizeof(payload) prevents buffer overflow.
        snprintf(payload, sizeof(payload),
                 "hello from esp32: %d",
                 counter++);

        // Send one UDP datagram.
        //
        // sock                       = socket handle
        // payload                    = pointer to bytes to send
        // strlen(payload)            = number of bytes to send
        // 0                          = flags, no special behavior
        // (struct sockaddr *)&dest_addr = destination address, cast to generic socket address
        // sizeof(dest_addr)          = size of destination address struct
        int err = sendto(
            sock,
            payload,
            strlen(payload),
            0,
            (struct sockaddr *)&dest_addr,
            sizeof(dest_addr)
        );

        // sendto returns the number of bytes sent, or -1 on error.
        if (err < 0) {
            ESP_LOGE(TAG, "sendto failed");
        } else {
            ESP_LOGI(TAG, "sent %d bytes: %s", err, payload);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

# Receiving the packets

```python
import socket as s

# Create an IPv4 UDP socket.
sock = s.socket(s.AF_INET, s.SOCK_DGRAM)

# Bind this socket to every local network interface
# on port 5005 on every ( 0.0.0.0 ) interface wifi, ethernet, loopback...
sock.bind(("0.0.0.0", 5005))

while True:
    # Wait until one UDP datagram arrives.
    # Returns:
    #   data -> bytes received
    #   addr -> (sender_ip, sender_port)
    
    # 2048 == max number of bytes we accept
    data, addr = sock.recvfrom(2048)

    print(addr, data)
```

## Esp32 monitor logs:

```text
--- esp-idf-monitor 1.9.0 on /dev/cu.usbmodem101 115200
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
I (30) boot: ESP-IDF v6.0.2 2nd stage bootloader
I (30) boot: compile time Jul  7 2026 12:20:27
I (31) boot: chip revision: v0.0
I (31) boot: efuse block revision: v0.2
I (31) boot.esp32s2: SPI Speed      : 80MHz
I (31) boot.esp32s2: SPI Mode       : DIO
I (31) boot.esp32s2: SPI Flash Size : 2MB
I (31) boot: Enabling RNG early entropy source...
I (32) boot: Partition Table:
I (32) boot: ## Label            Usage          Type ST Offset   Length
I (33) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (34) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (35) boot:  2 factory          factory app      00 00 00010000 00100000
I (36) boot: End of partition table
I (36) esp_image: segment 0: paddr=00010020 vaddr=3f000020 size=1abe8h (109544) map
I (61) esp_image: segment 1: paddr=0002ac10 vaddr=3ffc8050 size=03ec0h ( 16064) load
I (65) esp_image: segment 2: paddr=0002ead8 vaddr=40024000 size=01540h (  5440) load
I (67) esp_image: segment 3: paddr=00030020 vaddr=40080020 size=82694h (534164) map
I (180) esp_image: segment 4: paddr=000b26bc vaddr=40025540 size=12b0ch ( 76556) load
I (201) esp_image: segment 5: paddr=000c51d0 vaddr=50000000 size=00024h (    36) load
I (213) boot: Loaded app from partition at offset 0x10000
I (213) boot: Disabling RNG early entropy source...
I (235) main_task: Started on CPU0
I (235) main_task: Calling app_main()
Hello from ESP32
Led init
I (235) led: init
I (235) led: init done
Red led
Green led
Blue led
All leds ok
I (1755) wifi:wifi driver task: 3ffff6e0, prio:23, stack:6656, core=0
I (1775) wifi:wifi firmware version: 00ad238
I (1775) wifi:wifi certification version: v7.0
I (1775) wifi:config NVS flash: enabled
I (1775) wifi:config nano formatting: disabled
I (1775) wifi:Init data frame dynamic rx buffer num: 32
I (1775) wifi:Init static rx mgmt buffer num: 5
I (1775) wifi:Init management short buffer num: 32
I (1775) wifi:Init dynamic tx buffer num: 32
I (1775) wifi:Init static rx buffer size: 1600
I (1775) wifi:Init static rx buffer num: 10
I (1775) wifi:Init dynamic rx buffer num: 32
I (1775) wifi_init: rx ba win: 6
I (1775) wifi_init: accept mbox: 6
I (1775) wifi_init: tcpip mbox: 32
I (1775) wifi_init: udp mbox: 6
I (1775) wifi_init: tcp mbox: 6
I (1775) wifi_init: tcp tx win: 5760
I (1775) wifi_init: tcp rx win: 5760
I (1775) wifi_init: tcp mss: 1440
I (1775) wifi_init: WiFi IRAM OP enabled
I (1775) wifi_init: WiFi RX IRAM OP enabled
W (1775) wifi:Password length matches WPA2 standards, authmode threshold changes from OPEN to WPA2
I (1785) phy_init: phy_version 2623,ff3f498,Dec 17 2025,14:48:07
I (1835) wifi:mode : sta (84:f7:03:cd:dc:c4)
I (1835) wifi:enable tsf
I (1845) wifi_sta: wifi_init_sta finished
I (1845) wifi:new:<6,0>, old:<1,0>, ap:<255,255>, sta:<6,0>, prof:1, snd_ch_cfg:0x0
I (1855) wifi:state: init -> auth (0xb0)
I (1865) wifi:state: auth -> assoc (0x0)
I (1885) wifi:state: assoc -> run (0x10)
I (1925) wifi:connected with NOS-21BA, aid = 3, channel 6, BW20, bssid = 94:91:7f:32:21:ba
I (1935) wifi:security: WPA2-PSK, phy: bgn, rssi: -35, cipher(pairwise:0x3, group:0x1), pmf:0
I (1945) wifi:pm start, type: 1

I (1945) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (1955) wifi:dp: 2, bi: 102400, li: 4, scale listen interval from 307200 us to 409600 us
I (1965) wifi:AP's beacon interval = 102400 us, DTIM period = 2
I (2995) esp_netif_handlers: sta ip: 192.168.1.3, mask: 255.255.255.0, gw: 192.168.1.1
I (3005) wifi_sta: Got IP: 192.168.1.3
I (3005) udp_sender: sent: hello from esp32: 0
I (3005) main_task: Returned from app_main()
I (3225) wifi:<ba-add>idx:0 (ifx:0, 94:91:7f:32:21:ba), tid:0, ssn:1, winSize:64
I (4005) udp_sender: sent: hello from esp32: 1
I (5005) udp_sender: sent: hello from esp32: 2
I (6005) udp_sender: sent: hello from esp32: 3
I (7005) udp_sender: sent: hello from esp32: 4
```

## Python socket:

```text
 [main] > python listener.py
('192.168.1.3', 52284) b'hello from esp32: 0'
('192.168.1.3', 52284) b'hello from esp32: 1'
('192.168.1.3', 52284) b'hello from esp32: 2'
('192.168.1.3', 52284) b'hello from esp32: 3'
('192.168.1.3', 52284) b'hello from esp32: 4'
```

---

A specific event handler was added, in order for the UDP packet to not be sent before an IP was attributed.

```c
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifi_event_group;
...
# in the event handler callback after getting an IP
xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
...

# In the wifi init sta after initializing the nvs:
    wifi_event_group = xEventGroupCreate();
    configASSERT(wifi_event_group != NULL);
```

Hence now, the main.c function calls 

```c
void wifi_wait_connected(void)
{
    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,          // don't clear the bit afterwards
        pdTRUE,           // qait until all requested bits are set
        portMAX_DELAY     // wait forever ...
    );
}
```

Which is a bit weird because usually having a return value is better, but apparently this is how it is supposed to be done.
