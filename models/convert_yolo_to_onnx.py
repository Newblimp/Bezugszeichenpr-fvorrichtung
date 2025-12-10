#!/usr/bin/env python3
"""
Convert YOLO model to ONNX format

Usage:
    python convert_yolo_to_onnx.py <model_file> [--imgsz 640] [--simplify]

Examples:
    python convert_yolo_to_onnx.py best.pt
    python convert_yolo_to_onnx.py yolov8n.pt --imgsz 640 --simplify
"""

import argparse
import sys
from pathlib import Path

try:
    from ultralytics import YOLO
except ImportError:
    print("Error: ultralytics package not found.")
    print("Install it with: pip install ultralytics")
    sys.exit(1)


def convert_to_onnx(model_path, imgsz=640, simplify=False, dynamic=False):
    """
    Convert YOLO model to ONNX format

    Args:
        model_path: Path to the YOLO model (.pt file)
        imgsz: Input image size (default: 640)
        simplify: Whether to simplify the ONNX model (default: False)
        dynamic: Whether to use dynamic input shapes (default: False)

    Returns:
        Path to the exported ONNX file
    """
    model_path = Path(model_path)

    if not model_path.exists():
        print(f"Error: Model file '{model_path}' not found")
        sys.exit(1)

    print(f"Loading YOLO model from: {model_path}")
    model = YOLO(str(model_path))

    # Prepare export arguments
    export_args = {
        'format': 'onnx',
        'imgsz': imgsz,
        'simplify': simplify,
        'dynamic': dynamic,
    }

    print(f"Exporting to ONNX (imgsz={imgsz}, simplify={simplify}, dynamic={dynamic})...")
    onnx_path = model.export(**export_args)

    # Get the exported file path
    onnx_file = Path(onnx_path)

    # Calculate file sizes
    pt_size_mb = model_path.stat().st_size / (1024 * 1024)
    onnx_size_mb = onnx_file.stat().st_size / (1024 * 1024)

    print("\n" + "="*60)
    print("✓ Conversion successful!")
    print("="*60)
    print(f"Input file:  {model_path.name} ({pt_size_mb:.2f} MB)")
    print(f"Output file: {onnx_file.name} ({onnx_size_mb:.2f} MB)")
    print(f"Saved to:    {onnx_file.absolute()}")
    print("="*60)

    return onnx_file


def main():
    parser = argparse.ArgumentParser(
        description='Convert YOLO model to ONNX format',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s best.pt
  %(prog)s yolov8n.pt --imgsz 640 --simplify
  %(prog)s ../models/yolov11s.pt --simplify --dynamic
        """
    )

    parser.add_argument(
        'model',
        type=str,
        help='Path to YOLO model file (.pt)'
    )

    parser.add_argument(
        '--imgsz',
        type=int,
        default=640,
        help='Input image size (default: 640)'
    )

    parser.add_argument(
        '--simplify',
        action='store_true',
        help='Simplify ONNX model using onnx-simplifier (recommended for deployment)'
    )

    parser.add_argument(
        '--dynamic',
        action='store_true',
        help='Export with dynamic input shapes (allows variable batch size)'
    )

    args = parser.parse_args()

    try:
        convert_to_onnx(
            model_path=args.model,
            imgsz=args.imgsz,
            simplify=args.simplify,
            dynamic=args.dynamic
        )
    except Exception as e:
        print(f"\n❌ Error during conversion: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
