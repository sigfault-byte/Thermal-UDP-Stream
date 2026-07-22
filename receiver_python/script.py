import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import ListedColormap


MAGIC = b"PADAWAN\x00"
THERMAL_FORMAT = "<8sBBIIB"
THERMAL_HEADER_SIZE = struct.calcsize(THERMAL_FORMAT)
THERMAL_VERSION = 2
THERMAL_TYPE_FRAME = 1
WIDTH = 32
HEIGHT = 24
PIXELS = WIDTH * HEIGHT
THERMAL_PACKET_SIZE = THERMAL_HEADER_SIZE + PIXELS

QUANTIZATION_RANGES = {
    1: (10.0, 45.0),
    2: (20.0, 60.0),
    3: (0.0, 100.0),
}

assert THERMAL_HEADER_SIZE == 19
assert THERMAL_PACKET_SIZE == 787


@dataclass(frozen=True)
class ThermalPacket:
    byte_index: int
    frame_id: int
    timestamp_ms: int
    quantization_mode: int
    pixels: np.ndarray


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Read a raw thermal UDP .bin capture and save images for one packet."
        )
    )
    parser.add_argument("path", type=Path, help="Path to the raw .bin file")

    selector = parser.add_mutually_exclusive_group(required=True)
    selector.add_argument(
        "--byte",
        type=int,
        dest="byte_index",
        help="Packet start byte index in the .bin file",
    )
    selector.add_argument(
        "--frame-id",
        type=int,
        help="Frame ID to find by scanning packet headers",
    )

    parser.add_argument(
        "--output-dir",
        type=Path,
        help=(
            "Directory for output PNGs. Defaults to thermal_outputs next to the "
            "input file."
        ),
    )
    return parser.parse_args()


def verbose(message: str) -> None:
    print(message, flush=True)


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def read_capture(path: Path) -> bytes:
    if not path.exists():
        fail(f"input file does not exist: {path}")

    if not path.is_file():
        fail(f"input path is not a file: {path}")

    data = path.read_bytes()
    if not data:
        fail(f"input file is empty: {path}")

    if len(data) % THERMAL_PACKET_SIZE != 0:
        fail(
            f"input file size {len(data)} is not an exact multiple of "
            f"{THERMAL_PACKET_SIZE} bytes"
        )

    return data


def decode_header(data: bytes, byte_index: int) -> tuple[bytes, int, int, int, int, int]:
    header = data[byte_index : byte_index + THERMAL_HEADER_SIZE]
    if len(header) != THERMAL_HEADER_SIZE:
        fail(
            f"packet at byte {byte_index} has a short header: "
            f"{len(header)} byte(s)"
        )

    return struct.unpack(THERMAL_FORMAT, header)


def validate_header(
    magic: bytes,
    version: int,
    packet_type: int,
    quantization_mode: int,
    byte_index: int,
) -> None:
    if magic != MAGIC:
        fail(f"bad magic at byte {byte_index}: {magic!r}, expected {MAGIC!r}")

    if version != THERMAL_VERSION:
        fail(
            f"bad version at byte {byte_index}: {version}, "
            f"expected {THERMAL_VERSION}"
        )

    if packet_type != THERMAL_TYPE_FRAME:
        fail(
            f"bad packet type at byte {byte_index}: {packet_type}, "
            f"expected {THERMAL_TYPE_FRAME}"
        )

    if quantization_mode not in QUANTIZATION_RANGES:
        fail(
            f"unknown quantization mode at byte {byte_index}: "
            f"{quantization_mode}"
        )


def parse_packet(data: bytes, byte_index: int) -> ThermalPacket:
    if byte_index < 0:
        fail(f"--byte must be non-negative, got {byte_index}")

    if byte_index % THERMAL_PACKET_SIZE != 0:
        fail(
            f"--byte must be an exact packet start offset divisible by "
            f"{THERMAL_PACKET_SIZE}; got {byte_index}"
        )

    if byte_index + THERMAL_PACKET_SIZE > len(data):
        fail(
            f"packet at byte {byte_index} would extend past end of file "
            f"({len(data)} bytes)"
        )

    (
        magic,
        version,
        packet_type,
        frame_id,
        timestamp_ms,
        quantization_mode,
    ) = decode_header(data, byte_index)

    validate_header(magic, version, packet_type, quantization_mode, byte_index)

    pixel_start = byte_index + THERMAL_HEADER_SIZE
    pixel_end = byte_index + THERMAL_PACKET_SIZE
    pixels = np.frombuffer(data[pixel_start:pixel_end], dtype=np.uint8).reshape(
        HEIGHT,
        WIDTH,
    )

    return ThermalPacket(
        byte_index=byte_index,
        frame_id=frame_id,
        timestamp_ms=timestamp_ms,
        quantization_mode=quantization_mode,
        pixels=pixels.copy(),
    )


def find_packet_by_frame_id(data: bytes, frame_id: int) -> ThermalPacket:
    if frame_id < 0:
        fail(f"--frame-id must be non-negative, got {frame_id}")

    packet_count = len(data) // THERMAL_PACKET_SIZE
    for packet_index in range(packet_count):
        byte_index = packet_index * THERMAL_PACKET_SIZE
        (
            magic,
            version,
            packet_type,
            candidate_frame_id,
            _timestamp_ms,
            quantization_mode,
        ) = decode_header(data, byte_index)

        validate_header(magic, version, packet_type, quantization_mode, byte_index)

        if candidate_frame_id == frame_id:
            return parse_packet(data, byte_index)

    fail(f"frame ID {frame_id} was not found in {packet_count} packet(s)")


def decode_temperatures(pixels: np.ndarray, quantization_mode: int) -> np.ndarray:
    temp_min, temp_max = QUANTIZATION_RANGES[quantization_mode]
    span = temp_max - temp_min

    temps = np.empty(pixels.shape, dtype=np.float32)
    temps.fill(np.nan)

    valid = (pixels > 0) & (pixels < 255)
    temps[valid] = temp_min + ((pixels[valid].astype(np.float32) - 1.0) * span / 253.0)
    return temps


def base_output_path(output_dir: Path, packet: ThermalPacket) -> Path:
    return output_dir / f"frame_{packet.frame_id:06d}_byte_{packet.byte_index}"


def decorate_grid(ax: plt.Axes, title: str) -> None:
    ax.set_title(title)
    ax.set_xticks(np.arange(WIDTH))
    ax.set_yticks(np.arange(HEIGHT))
    ax.set_xticks(np.arange(-0.5, WIDTH, 1), minor=True)
    ax.set_yticks(np.arange(-0.5, HEIGHT, 1), minor=True)
    ax.grid(which="minor", color="white", linewidth=0.25, alpha=0.35)
    ax.tick_params(which="minor", bottom=False, left=False)
    ax.tick_params(axis="both", labelsize=6)
    ax.set_xlabel("x")
    ax.set_ylabel("y")


def save_raw_byte_image(packet: ThermalPacket, output_path: Path) -> None:
    fig, ax = plt.subplots(figsize=(10, 7), constrained_layout=True)
    image = ax.imshow(
        packet.pixels,
        cmap="gray",
        interpolation="nearest",
        vmin=0,
        vmax=255,
    )
    decorate_grid(
        ax,
        (
            f"Raw bytes | frame {packet.frame_id} | byte {packet.byte_index} | "
            f"mode {packet.quantization_mode}"
        ),
    )
    colorbar = fig.colorbar(image, ax=ax)
    colorbar.set_label("byte value")
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def save_temperature_value_image(
    packet: ThermalPacket,
    temps: np.ndarray,
    output_path: Path,
) -> None:
    valid = np.isfinite(temps)
    if valid.any():
        vmin = float(np.nanmin(temps))
        vmax = float(np.nanmax(temps))
    else:
        vmin = 0.0
        vmax = 1.0

    fig, ax = plt.subplots(figsize=(16, 10), constrained_layout=True)
    image = ax.imshow(
        np.ma.masked_invalid(temps),
        cmap="viridis",
        interpolation="nearest",
        vmin=vmin,
        vmax=vmax,
    )
    decorate_grid(
        ax,
        (
            f"Decoded temperatures | frame {packet.frame_id} | "
            f"byte {packet.byte_index}"
        ),
    )

    for y in range(HEIGHT):
        for x in range(WIDTH):
            byte_value = int(packet.pixels[y, x])
            temp = temps[y, x]
            if byte_value == 0:
                label = "LOW"
            elif byte_value == 255:
                label = "HIGH"
            elif np.isfinite(temp):
                label = f"{temp:.1f}"
            else:
                label = "?"

            ax.text(
                x,
                y,
                label,
                ha="center",
                va="center",
                fontsize=4.5,
                color="white" if valid.any() and np.isfinite(temp) else "black",
            )

    colorbar = fig.colorbar(image, ax=ax)
    colorbar.set_label("Celsius")
    fig.savefig(output_path, dpi=180)
    plt.close(fig)


def save_temperature_color_image(
    packet: ThermalPacket,
    temps: np.ndarray,
    output_path: Path,
) -> tuple[float, float]:
    valid_values = temps[np.isfinite(temps)]
    if valid_values.size == 0:
        vmin = 0.0
        vmax = 1.0
    else:
        vmin = float(np.percentile(valid_values, 5))
        vmax = float(np.percentile(valid_values, 95))
        if vmax <= vmin:
            vmin = float(valid_values.min())
            vmax = float(valid_values.max())
        if vmax <= vmin:
            vmax = vmin + 1.0

    display = temps.copy()
    display[packet.pixels == 0] = vmin - 1.0
    display[packet.pixels == 255] = vmax + 1.0

    inferno = plt.get_cmap("inferno", 254)
    colors = np.vstack(
        [
            np.array([[1.0, 1.0, 1.0, 1.0]]),
            inferno(np.linspace(0.0, 1.0, 254)),
            np.array([[0.0, 0.0, 0.0, 1.0]]),
        ]
    )
    cmap = ListedColormap(colors)

    fig, ax = plt.subplots(figsize=(10, 7), constrained_layout=True)
    image = ax.imshow(
        display,
        cmap=cmap,
        interpolation="nearest",
        vmin=vmin - 1.0,
        vmax=vmax + 1.0,
    )
    decorate_grid(
        ax,
        (
            f"Temperature colors | frame {packet.frame_id} | "
            f"byte {packet.byte_index} | vmin={vmin:.2f}C vmax={vmax:.2f}C"
        ),
    )
    colorbar = fig.colorbar(image, ax=ax)
    colorbar.set_label("Celsius; white=0 reserved, black=255 reserved")
    fig.savefig(output_path, dpi=160)
    plt.close(fig)

    return vmin, vmax


def save_images(packet: ThermalPacket, output_dir: Path) -> list[Path]:
    output_dir.mkdir(parents=True, exist_ok=True)
    base = base_output_path(output_dir, packet)
    raw_path = base.with_name(base.name + "_raw.png")
    values_path = base.with_name(base.name + "_temps_values.png")
    colors_path = base.with_name(base.name + "_temps_colors.png")

    temps = decode_temperatures(packet.pixels, packet.quantization_mode)
    save_raw_byte_image(packet, raw_path)
    save_temperature_value_image(packet, temps, values_path)
    vmin, vmax = save_temperature_color_image(packet, temps, colors_path)

    verbose(f"Temperature color scale from selected frame: vmin={vmin:.3f}C vmax={vmax:.3f}C")
    return [raw_path, values_path, colors_path]


def main() -> int:
    args = parse_args()
    path = args.path
    output_dir = args.output_dir or (path.parent / "thermal_outputs")

    verbose(f"Reading capture: {path}")
    data = read_capture(path)
    packet_count = len(data) // THERMAL_PACKET_SIZE
    verbose(f"File size: {len(data)} bytes")
    verbose(f"Packet size: {THERMAL_PACKET_SIZE} bytes")
    verbose(f"Packet count: {packet_count}")

    if args.byte_index is not None:
        verbose(f"Selecting packet by byte index: {args.byte_index}")
        packet = parse_packet(data, args.byte_index)
    else:
        verbose(f"Selecting packet by frame ID: {args.frame_id}")
        packet = find_packet_by_frame_id(data, args.frame_id)

    temp_min, temp_max = QUANTIZATION_RANGES[packet.quantization_mode]
    verbose("Selected packet:")
    verbose(f"  byte index: {packet.byte_index}")
    verbose(f"  frame ID: {packet.frame_id}")
    verbose(f"  timestamp: {packet.timestamp_ms} ms")
    verbose(
        f"  quantization mode: {packet.quantization_mode} "
        f"({temp_min:g}C..{temp_max:g}C)"
    )
    verbose(f"  pixel shape: {HEIGHT} x {WIDTH}")

    paths = save_images(packet, output_dir)
    verbose("Wrote images:")
    for output_path in paths:
        verbose(f"  {output_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
