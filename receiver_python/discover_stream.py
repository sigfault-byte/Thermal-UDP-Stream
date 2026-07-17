import socket
import struct

import matplotlib.pyplot as plt
import numpy as np


# The ESP32 periodically broadcasts its presence here. This is discovery only:
# after we accept a valid packet, we use the sender IP to open the TCP command
# channel and ask the firmware to start the real thermal stream.
DISCOVERY_HOST = "0.0.0.0"
DISCOVERY_PORT = 5004

# Thermal frames are sent by the ESP32 to the machine that sent START. The
# firmware uses UDP 5005 for the high-rate frame data.
STREAM_HOST = "0.0.0.0"
STREAM_PORT = 5005

# TCP 5006 is the firmware command channel. START over TCP tells the ESP32 to
# begin sending UDP frames back to this computer.
COMMAND_PORT = 5006
SOCKET_TIMEOUT_SECONDS = 5.0

MAGIC = b"PADAWAN\x00"

# The discovery packet is obfuscated with a repeating XOR key. This is not
# crypto; it just hides the clear PADAWAN marker on the wire.
HANDSHAKE_SECRET = b"padawan"
HANDSHAKE_FORMAT = "<8sBB"
HANDSHAKE_SIZE = struct.calcsize(HANDSHAKE_FORMAT)
HANDSHAKE_VERSION = 1
HANDSHAKE_TYPE_DISCOVERY = 1

# TCP command packet format:
#   magic[8], version, command, value
# The START command does not need a value, so the last byte is zero.
COMMAND_FORMAT = "<8sBBB"
COMMAND_RESPONSE_FORMAT = "<8sBBB"
COMMAND_SIZE = struct.calcsize(COMMAND_FORMAT)
COMMAND_RESPONSE_SIZE = struct.calcsize(COMMAND_RESPONSE_FORMAT)
COMMAND_VERSION = 1
COMMAND_START = 1
COMMAND_STATUS_OK = 1

# Thermal frame format, protocol v2:
#   magic[8], version, type, frame_id, timestamp_ms, quantization_mode
# The 768 payload bytes that follow are one uint8 per MLX90640 pixel.
THERMAL_FORMAT = "<8sBBIIB"
THERMAL_HEADER_SIZE = struct.calcsize(THERMAL_FORMAT)
THERMAL_VERSION = 2
THERMAL_TYPE_FRAME = 1
WIDTH = 32
HEIGHT = 24
PIXELS = WIDTH * HEIGHT
THERMAL_PACKET_SIZE = THERMAL_HEADER_SIZE + PIXELS

# The firmware maps float temperatures into byte values using one of these
# ranges. Byte 0 means "below range"; byte 255 means "above range".
QUANTIZATION_RANGES = {
    1: (10.0, 45.0),
    2: (20.0, 60.0),
    3: (0.0, 100.0),
}

# Keep the protocol sizes obvious and fail early if someone edits a format.
assert HANDSHAKE_SIZE == 10
assert COMMAND_SIZE == 11
assert COMMAND_RESPONSE_SIZE == 11
assert THERMAL_HEADER_SIZE == 19
assert THERMAL_PACKET_SIZE == 787


def hex_bytes(data):
    """Return a compact hex dump for verbose protocol logging."""
    return " ".join(f"{byte:02X}" for byte in data)


def xor_handshake(data):
    """Decode the ESP32 discovery packet using the repeating padawan key."""
    return bytes(
        byte ^ HANDSHAKE_SECRET[index % len(HANDSHAKE_SECRET)]
        for index, byte in enumerate(data)
    )


def wait_for_discovery():
    """Block until a valid ESP32 discovery packet arrives on UDP 5004."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((DISCOVERY_HOST, DISCOVERY_PORT))

    print(
        f"Listening for UDP discovery packets on "
        f"{DISCOVERY_HOST}:{DISCOVERY_PORT}"
    )

    while True:
        data, addr = sock.recvfrom(2048)
        sender_ip, sender_port = addr

        print(
            f"Received UDP discovery candidate: "
            f"{len(data)} byte(s) from {sender_ip}:{sender_port}"
        )

        if len(data) != HANDSHAKE_SIZE:
            print(f"  Rejected: expected {HANDSHAKE_SIZE} bytes")
            continue

        decoded = xor_handshake(data)
        magic, version, packet_type = struct.unpack(HANDSHAKE_FORMAT, decoded)

        print(
            "  Decoded handshake: "
            f"magic={magic!r} version={version} type={packet_type}"
        )

        if magic != MAGIC:
            print(f"  Rejected: bad magic, decoded bytes={hex_bytes(decoded)}")
            continue

        if version != HANDSHAKE_VERSION:
            print(f"  Rejected: unsupported handshake version {version}")
            continue

        if packet_type != HANDSHAKE_TYPE_DISCOVERY:
            print(f"  Rejected: unsupported handshake type {packet_type}")
            continue

        print(f"Discovered ESP32 at {sender_ip}")
        sock.close()
        return sender_ip


def send_start_command(esp_ip):
    """Open TCP 5006 and send the START command packet."""
    command = struct.pack(
        COMMAND_FORMAT,
        MAGIC,
        COMMAND_VERSION,
        COMMAND_START,
        0,
    )

    print(f"Connecting to {esp_ip}:{COMMAND_PORT} over TCP")

    with socket.create_connection(
        (esp_ip, COMMAND_PORT),
        timeout=SOCKET_TIMEOUT_SECONDS,
    ) as sock:
        print(f"Sending START command bytes: {hex_bytes(command)}")
        sock.sendall(command)

        # The response mirrors the command packet layout, replacing value with
        # status. A short timeout keeps a broken command server from hanging the
        # script forever.
        sock.settimeout(SOCKET_TIMEOUT_SECONDS)
        response = sock.recv(COMMAND_RESPONSE_SIZE)

    if len(response) != COMMAND_RESPONSE_SIZE:
        print(
            f"TCP response warning: expected {COMMAND_RESPONSE_SIZE} bytes, "
            f"got {len(response)} byte(s): {hex_bytes(response)}"
        )
        return

    magic, version, command_echo, status = struct.unpack(
        COMMAND_RESPONSE_FORMAT,
        response,
    )

    print(f"Received TCP response bytes: {hex_bytes(response)}")
    print(
        "Decoded TCP response: "
        f"magic={magic!r} version={version} "
        f"command={command_echo} status={status}"
    )

    if magic != MAGIC:
        print("TCP response warning: bad magic")
    elif version != COMMAND_VERSION:
        print(f"TCP response warning: unsupported version {version}")
    elif command_echo != COMMAND_START:
        print(f"TCP response warning: response echoed command {command_echo}")
    elif status != COMMAND_STATUS_OK:
        print(f"TCP response warning: START failed with status {status}")
    else:
        print("TCP START response status is OK")


def decode_pixels(pixels, quantization_mode):
    """Convert quantized uint8 pixels back into approximate Celsius values."""
    temp_min, temp_max = QUANTIZATION_RANGES[quantization_mode]
    span = temp_max - temp_min

    temps = np.empty_like(pixels, dtype=np.float32)
    temps[pixels == 0] = np.nan
    temps[pixels == 255] = np.nan

    mask = (pixels > 0) & (pixels < 255)
    temps[mask] = temp_min + ((pixels[mask] - 1) * span / 253.0)

    return temps


def plot_stream():
    """Receive UDP 5005 thermal frames and display them with matplotlib."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((STREAM_HOST, STREAM_PORT))

    print(f"Listening for thermal frames on UDP {STREAM_HOST}:{STREAM_PORT}")

    plt.ion()
    fig, ax = plt.subplots()
    image = ax.imshow(
        np.zeros((HEIGHT, WIDTH)),
        vmin=20,
        vmax=40,
        cmap="inferno",
        interpolation="bicubic",
    )
    plt.colorbar(image, ax=ax)
    ax.set_title("Thermal stream waiting for frames")

    while True:
        data, addr = sock.recvfrom(8192)

        if len(data) != THERMAL_PACKET_SIZE:
            print(
                f"Wrong thermal packet size from {addr[0]}:{addr[1]}: "
                f"{len(data)} byte(s), expected {THERMAL_PACKET_SIZE}"
            )
            continue

        (
            magic,
            version,
            packet_type,
            frame_id,
            timestamp_ms,
            quantization_mode,
        ) = struct.unpack(THERMAL_FORMAT, data[:THERMAL_HEADER_SIZE])

        if magic != MAGIC:
            print(f"Bad thermal magic from {addr[0]}:{addr[1]}: {magic!r}")
            continue

        if version != THERMAL_VERSION or packet_type != THERMAL_TYPE_FRAME:
            print(
                f"Bad thermal version/type from {addr[0]}:{addr[1]}: "
                f"version={version} type={packet_type}"
            )
            continue

        if quantization_mode not in QUANTIZATION_RANGES:
            print(
                f"Unknown quantization mode from {addr[0]}:{addr[1]}: "
                f"{quantization_mode}"
            )
            continue

        pixels = np.frombuffer(
            data[THERMAL_HEADER_SIZE:],
            dtype=np.uint8,
        ).reshape(HEIGHT, WIDTH)
        temps = decode_pixels(pixels, quantization_mode)
        valid = temps[np.isfinite(temps)]

        if valid.size > 0:
            vmin = np.percentile(valid, 5)
            vmax = np.percentile(valid, 95)

            if vmax > vmin:
                image.set_clim(vmin, vmax)

        image.set_data(temps)
        ax.set_title(
            f"Frame {frame_id} | {timestamp_ms} ms | "
            f"mode {quantization_mode} | from {addr[0]}"
        )
        plt.pause(0.001)


def main():
    esp_ip = wait_for_discovery()
    send_start_command(esp_ip)
    plot_stream()


if __name__ == "__main__":
    main()
