import socket
import struct

import matplotlib.pyplot as plt
import numpy as np

PORT = 5005

MAGIC = b"PADAWAN\x00"
VERSION = 1
TYPE_FRAME = 1

WIDTH = 32
HEIGHT = 24
PIXELS = WIDTH * HEIGHT

HEADER_FORMAT = "<8sBBII"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
PACKET_SIZE = HEADER_SIZE + PIXELS


def decode_pixels(pixels):
    temps = np.empty_like(pixels, dtype=np.float32)

    temps[pixels == 0] = np.nan  # below 10
    temps[pixels == 255] = np.nan  # above 45

    mask = (pixels > 0) & (pixels < 255)
    temps[mask] = 10.0 + ((pixels[mask] - 1) * 35.0 / 253.0)

    return temps


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", PORT))

plt.ion()
fig, ax = plt.subplots()
image = ax.imshow(
    np.zeros((24, 32)),
    vmin=20,
    vmax=40,
    cmap="inferno",
    interpolation="bicubic",
)
plt.colorbar(image, ax=ax)
ax.set_title("Thermal stream")

while True:
    data, addr = sock.recvfrom(8192)

    if len(data) != PACKET_SIZE:
        print(f"Wrong size: {len(data)}")
        continue

    magic, version, packet_type, frame_id, timestamp_ms = struct.unpack(
        HEADER_FORMAT,
        data[:HEADER_SIZE],
    )

    if magic.rstrip(b"\x00") != b"PADAWAN":
        print(f"Bad magic: {magic!r}")
        continue

    if version != VERSION or packet_type != TYPE_FRAME:
        print("Bad version/type")
        continue

    pixels = np.frombuffer(data[HEADER_SIZE:], dtype=np.uint8).reshape(HEIGHT, WIDTH)
    temps = decode_pixels(pixels)
    valid = temps[np.isfinite(temps)]

    if valid.size > 0:
        vmin = np.percentile(valid, 5)
        vmax = np.percentile(valid, 95)

        if vmax > vmin:
            image.set_clim(vmin, vmax)

    image.set_data(temps)
    ax.set_title(f"Frame {frame_id} | {timestamp_ms} ms | from {addr[0]}")
    plt.pause(0.001)
