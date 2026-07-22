import argparse
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")

import matplotlib.image as mpimg
import numpy as np


DEFAULT_SIGMA = 2.0
DEFAULT_AMOUNT = 1.5


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Apply blur-subtraction image processing to a PNG. Saves a Gaussian "
            "blur, a high-pass edge/detail map, and an unsharp-mask sharpened "
            "image."
        )
    )
    parser.add_argument("path", type=Path, help="Path to the input .png image")
    parser.add_argument(
        "--sigma",
        type=float,
        default=DEFAULT_SIGMA,
        help=f"Gaussian blur sigma in pixels. Default: {DEFAULT_SIGMA}",
    )
    parser.add_argument(
        "--amount",
        type=float,
        default=DEFAULT_AMOUNT,
        help=f"Unsharp mask strength. Default: {DEFAULT_AMOUNT}",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Directory for output PNGs. Defaults to image_outputs next to input.",
    )
    return parser.parse_args()


def verbose(message: str) -> None:
    print(message, flush=True)


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def validate_args(path: Path, sigma: float, amount: float) -> None:
    if path.suffix.lower() != ".png":
        fail(f"input must be a .png file: {path}")

    if not path.exists():
        fail(f"input file does not exist: {path}")

    if not path.is_file():
        fail(f"input path is not a file: {path}")

    if sigma <= 0:
        fail(f"--sigma must be positive, got {sigma}")

    if amount < 0:
        fail(f"--amount must be non-negative, got {amount}")


def load_image(path: Path) -> tuple[np.ndarray, str]:
    image = mpimg.imread(path)
    original_dtype = str(image.dtype)
    image = np.asarray(image)

    if image.ndim not in (2, 3):
        fail(f"unsupported image shape {image.shape}; expected grayscale, RGB, or RGBA")

    if image.ndim == 3 and image.shape[2] not in (3, 4):
        fail(f"unsupported channel count {image.shape[2]}; expected RGB or RGBA")

    if np.issubdtype(image.dtype, np.integer):
        info = np.iinfo(image.dtype)
        image = image.astype(np.float32) / float(info.max)
    else:
        image = image.astype(np.float32)

    image = np.clip(image, 0.0, 1.0)
    return image, original_dtype


def split_color_and_alpha(image: np.ndarray) -> tuple[np.ndarray, np.ndarray | None]:
    if image.ndim == 2:
        return image, None

    if image.shape[2] == 4:
        return image[:, :, :3], image[:, :, 3]

    return image, None


def recombine_color_and_alpha(color: np.ndarray, alpha: np.ndarray | None) -> np.ndarray:
    if alpha is None:
        return color

    return np.dstack([color, alpha])


def gaussian_kernel_1d(sigma: float) -> np.ndarray:
    radius = max(1, int(np.ceil(sigma * 3.0)))
    x = np.arange(-radius, radius + 1, dtype=np.float32)
    kernel = np.exp(-(x * x) / (2.0 * sigma * sigma))
    kernel /= kernel.sum()
    return kernel.astype(np.float32)


def convolve_axis_reflect(image: np.ndarray, kernel: np.ndarray, axis: int) -> np.ndarray:
    radius = len(kernel) // 2
    pad_width: list[tuple[int, int]] = [(0, 0)] * image.ndim
    pad_width[axis] = (radius, radius)
    padded = np.pad(image, pad_width, mode="reflect")
    result = np.zeros_like(image, dtype=np.float32)

    for kernel_index, weight in enumerate(kernel):
        start = kernel_index
        end = start + image.shape[axis]
        slices = [slice(None)] * image.ndim
        slices[axis] = slice(start, end)
        result += weight * padded[tuple(slices)]

    return result


def gaussian_blur(image: np.ndarray, sigma: float) -> np.ndarray:
    kernel = gaussian_kernel_1d(sigma)
    blurred = convolve_axis_reflect(image, kernel, axis=0)
    blurred = convolve_axis_reflect(blurred, kernel, axis=1)
    return blurred


def safe_token(value: float) -> str:
    return f"{value:g}".replace("-", "neg").replace(".", "p")


def output_paths(
    input_path: Path,
    output_dir: Path,
    sigma: float,
    amount: float,
) -> tuple[Path, Path, Path]:
    sigma_token = safe_token(sigma)
    amount_token = safe_token(amount)
    base = output_dir / input_path.stem
    return (
        base.with_name(f"{base.name}_blur_sigma_{sigma_token}.png"),
        base.with_name(f"{base.name}_highpass_sigma_{sigma_token}.png"),
        base.with_name(
            f"{base.name}_unsharp_sigma_{sigma_token}_amount_{amount_token}.png"
        ),
    )


def save_image(path: Path, image: np.ndarray) -> None:
    mpimg.imsave(path, np.clip(image, 0.0, 1.0))


def main() -> int:
    args = parse_args()
    validate_args(args.path, args.sigma, args.amount)

    output_dir = args.output_dir or (args.path.parent / "image_outputs")
    output_dir.mkdir(parents=True, exist_ok=True)

    verbose("Technique names:")
    verbose("  Unsharp masking: original + amount * (original - blur)")
    verbose("  High-pass/detail map: original - blur")
    verbose(f"Reading image: {args.path}")

    image, original_dtype = load_image(args.path)
    color, alpha = split_color_and_alpha(image)

    verbose(f"Input shape: {image.shape}")
    verbose(f"Input dtype from reader: {original_dtype}")
    verbose("Normalized working range: 0.0..1.0 float32")
    verbose(f"Gaussian sigma: {args.sigma}")
    verbose(f"Unsharp amount: {args.amount}")
    if alpha is not None:
        verbose("Alpha channel detected: preserving alpha unchanged")

    blur_color = gaussian_blur(color, args.sigma)
    highpass = color - blur_color
    highpass_display = np.clip(highpass + 0.5, 0.0, 1.0)
    unsharp = np.clip(color + args.amount * highpass, 0.0, 1.0)

    blur_path, highpass_path, unsharp_path = output_paths(
        args.path,
        output_dir,
        args.sigma,
        args.amount,
    )

    save_image(blur_path, recombine_color_and_alpha(blur_color, alpha))
    save_image(highpass_path, recombine_color_and_alpha(highpass_display, alpha))
    save_image(unsharp_path, recombine_color_and_alpha(unsharp, alpha))

    verbose("Wrote images:")
    verbose(f"  blur: {blur_path}")
    verbose(f"  high-pass edge/detail map: {highpass_path}")
    verbose(f"  unsharp mask: {unsharp_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
