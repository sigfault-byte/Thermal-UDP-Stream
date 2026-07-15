# **ESP32 Thermal UDP Stream**

The purpose of this project is to build an embedded thermal imaging system that acquires data from an MLX90640 infrared sensor, converts the raw measurements into calibrated temperatures using the official Melexis library, quantizes the resulting thermal frame into a compact binary representation, and streams the data over UDP to real-time Python and C++/Qt viewers. In parallel, the ESP32-S2 exposes a dedicated TCP control interface that accepts commands such as starting or stopping the stream and changing the quantization mode at runtime, separating high-throughput data streaming from reliable configuration and control.


### Hardware
```text
Microcontroller : ESP32-S2 (Flipper Zero Wi-Fi Dev Board)
Thermal camera  : MLX90640-D55 (24×32 IR array)
Receiver        : Any computer capable of running Python | Streams complete thermal frames over Wi-Fi to a desktop receiver.
```

### Environment

- ESP-IDF
- Python with `uv`
OR
- Qt 6.8+ with Qt Quick, Qt Network, and Qt Graphs 
- CMake 3.16+


[ESP-IDF installation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)  
`https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/`


## Overview

A custom firmware was developed using the ESP-IDF framework.

The firmware:

- Configures the ESP32-S2 and MLX90640 over I²C (SDA on GPIO33, SCL on GPIO14).
- Periodically broadcasts an obfuscated UDP discovery packet, allowing compatible viewers to automatically detect the device.
- Reads and parses the sensor’s factory calibration EEPROM.
- Uses the official Melexis API to convert raw sensor data into calibrated object temperatures.
- Merges the two MLX90640 chessboard subpages into complete thermal frames.
- Quantizes the temperature data into a compact one-byte-per-pixel representation.
- Encapsulates each frame in a custom binary application protocol.
- Streams the resulting packets over Wi-Fi using UDP.
- Exposes a dedicated TCP control interface for runtime configuration (start/stop streaming, quantization mode, refresh rate, etc.).

The default quantization scheme is:
```
< 10°C      -> 0
10–45°C     -> 1–254
> 45°C      -> 255
```

This representation allows an entire 32 × 24 thermal frame (768 pixels), together with its protocol header, to fit comfortably within a single UDP datagram.

## C++ / Qt Viewer

The desktop viewer is implemented in modern C++ using Qt 6 and QML.

The application automatically discovers the device, receives and validates UDP datagrams, decodes the custom binary protocol, and renders the thermal image in real time. It provides both rolling and fixed display scales, computes live frame statistics (minimum, maximum, mean, and valid pixel counts), performs lightweight hotspot detection, and displays real-time temperature plots for both the frame mean and the detected hotspot.

The viewer also communicates with the ESP32 through the TCP control channel, allowing runtime configuration of parameters such as the quantization mode, streaming state, and sensor refresh rate without interrupting the data stream.

![qt_viewer](pictures/qt_viewer.png)

## Python Viewer

The Python receiver reconstructs the temperature values and displays the thermal image using Matplotlib with bicubic interpolation.

![hand](pictures/hand.png)
![computer](pictures/computer.png)

The MLX90640 operates at its factory default refresh rate of **2 Hz**.

# **Running**

Configure the Wi-Fi credentials in: 
```text
/main/wifi/wifi_sta.c
``` 

Update the receiver IP address in:
```text
`main/network/udp_sender.c`
```

Build the firmware:
```text
idf.py build
```

Flash the board 
```text
idf.py -p /dev/cu.XXX flash monitor
```

Start the C++ / Qt viewer
The viewer requires Qt 6.8 or later with the Quick, Network, and
Graphs modules.
```bash
cd viewer
cmake -S . -B build
cmake --build build
./build/padawan_viewer
./build/padawan_viewer -h
```

Start python receiver
```text
cd receiver_python

uv sync
uv run stream.py
```

## Future Work
```text
- Adaptive color palettes and dynamic range visualization.
- Temporal filtering and noise reduction.
- Thermal motion detection on thermal time series.
- UDP broadcast discovery instead of a fixed receiver IP.
- Higher frame rates.
- Export of recorded thermal sequences for offline analysis.
```

## Notes

`journals/` contains personal development notes, implementation decisions, encountered issues, and lessons learned.
