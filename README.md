# **ESP32 Thermal UDP Stream**

The purpose of this project is to build an embedded thermal-imaging system that streams quantized MLX90640 frames over UDP to live Python and Qt viewers.”

An ESP32-S2 reads an MLX90640 thermal camera, converts the raw
measurements into calibrated temperatures using the official Melexis
library, quantizes the temperatures into a compact binary format, and
streams complete thermal frames over UDP to either a C++/Qt viewer or
a lightweight Python receiver.


### Hardware
```text
Microcontroller : ESP32-S2 (Flipper Zero Wi-Fi Dev Board)
Thermal camera  : MLX90640-D55 (24×32 IR array)
Receiver        : Any computer capable of running Python | * Streams complete thermal frames over Wi-Fi to a desktop receiver.
```

### Environment

- ESP-IDF
- Python with `uv`
OR
- Qt 6.8+ with Qt Quick, Qt Network, and Qt Graphs 
- CMake 3.16+


[ESP-IDF installation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)  
`https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/`


## **Overview**

A custom firmware was developed using the ESP-IDF SDK.

The firmware:

- Configures the ESP32 and MLX90640 over I²C (SDA on IO33 SCL on IO14).
- Reads the factory calibration EEPROM.
- Uses the official Melexis API to compute calibrated object temperatures.
- Merges the two chessboard subpages into a complete thermal frame.
- Quantizes temperatures into a single byte per pixel.
- Packs the frame into a custom UDP protocol.
- Streams the packet over Wi-Fi to a Python receiver.

The quantization uses:

```
< 10°C      -> 0
10–45°C     -> 1–254
> 45°C      -> 255
```

allowing the complete **24×32** thermal frame (768 pixels) to fit comfortably inside a single UDP packet.

## C++ / Qt Viewer

The C++ receiver requires installing [Qt](https://doc.qt.io/qt-6/get-and-install-qt-cli.html).

The viewer receives UDP datagrams, validates and decodes them, displays the thermal frame through a QML interface, computes basic frame analytics, supports both rolling and fixed display scales, performs a simple hotspot detection, and plots a live time series of the frame mean temperature and hotspot temperature.

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

## Design Notes

`journals/` contains personal development notes, implementation decisions, encountered issues, and lessons learned.
