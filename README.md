# **ESP32 Thermal UDP Stream**

The purpose of this project is to build an embedded system capable of broadcasting thermal images over Wi-Fi.

An ESP32-S2 reads an MLX90640 thermal camera, converts the raw measurements into calibrated temperatures using the official Melexis library, quantizes the temperatures into a compact binary format, and streams them over UDP to a Python receiver for visualization and future analysis.


### Hardware
```text
Microcontroller : ESP32-S2 (Flipper Zero Wi-Fi Dev Board)
Thermal camera  : MLX90640-D55 (24×32 IR array)
Receiver        : Any computer capable of running Python
```

### Environment
```txt
ESP-IDF
uv
```

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

The quantizasion uses:

```
< 10°C      -> 0
10–45°C     -> 1–254
> 45°C      -> 255
```

allowing the complete **24×32** thermal frame (768 pixels) to fit comfortably inside a single UDP packet.

The Python receiver reconstructs the temperatures and displays the thermal image using Matplotlib with bicubic interpolation.

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
