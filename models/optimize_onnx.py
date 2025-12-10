#!/usr/bin/env python3
"""
ONNX Model Optimization Script

Reduces ONNX model size through various optimization techniques:
- Dynamic quantization (INT8)
- FP16 conversion
- Model simplification
- Graph optimization
- Static quantization (with calibration data)

Usage:
    python optimize_onnx.py <input.onnx> [options]

Examples:
    # Dynamic quantization (recommended for most cases)
    python optimize_onnx.py YOLOv11n.onnx --quantize dynamic

    # FP16 conversion (good balance of size and accuracy)
    python optimize_onnx.py YOLOv11n.onnx --fp16

    # Multiple optimizations
    python optimize_onnx.py YOLOv11n.onnx --simplify --fp16

    # Full optimization pipeline
    python optimize_onnx.py YOLOv11n.onnx --all
"""

import argparse
import sys
from pathlib import Path
import shutil

try:
    import onnx
    from onnx import version_converter, shape_inference
except ImportError:
    print("Error: onnx package not found.")
    print("Install it with: pip install onnx")
    sys.exit(1)


def get_model_size_mb(path):
    """Get model file size in MB"""
    return path.stat().st_size / (1024 * 1024)


def simplify_model(input_path, output_path):
    """
    Simplify ONNX model using onnx-simplifier
    Removes redundant nodes and constant folding
    """
    try:
        import onnxsim
    except ImportError:
        print("Warning: onnxsim not installed. Skipping simplification.")
        print("Install with: pip install onnx-simplifier")
        return False

    print("\n" + "="*60)
    print("SIMPLIFICATION")
    print("="*60)

    model = onnx.load(str(input_path))

    try:
        model_simp, check = onnxsim.simplify(model)
        if check:
            onnx.save(model_simp, str(output_path))
            print("✓ Model simplified successfully")
            return True
        else:
            print("⚠ Simplification check failed")
            return False
    except Exception as e:
        print(f"⚠ Simplification error: {e}")
        return False


def convert_fp16(input_path, output_path):
    """
    Convert FP32 model to FP16
    Reduces size by ~50% with minimal accuracy loss
    """
    print("\n" + "="*60)
    print("FP16 CONVERSION")
    print("="*60)

    try:
        from onnxconverter_common import float16
    except ImportError:
        print("Warning: onnxconverter-common not installed. Skipping FP16 conversion.")
        print("Install with: pip install onnxconverter-common")
        return False

    model = onnx.load(str(input_path))

    try:
        model_fp16 = float16.convert_float_to_float16(model, keep_io_types=True)
        onnx.save(model_fp16, str(output_path))
        print("✓ Converted to FP16 successfully")
        return True
    except Exception as e:
        print(f"⚠ FP16 conversion error: {e}")
        return False


def quantize_dynamic(input_path, output_path):
    """
    Apply dynamic quantization (INT8)
    Reduces model size and speeds up inference
    """
    print("\n" + "="*60)
    print("DYNAMIC QUANTIZATION (INT8)")
    print("="*60)

    try:
        from onnxruntime.quantization import quantize_dynamic, QuantType
    except ImportError:
        print("Warning: onnxruntime not installed. Skipping quantization.")
        print("Install with: pip install onnxruntime")
        return False

    try:
        quantize_dynamic(
            str(input_path),
            str(output_path),
            weight_type=QuantType.QInt8
        )
        print("✓ Dynamic quantization successful")
        return True
    except Exception as e:
        print(f"⚠ Quantization error: {e}")
        return False


def quantize_static(input_path, output_path, calibration_data_path=None):
    """
    Apply static quantization (requires calibration data)
    Best compression but needs representative data
    """
    print("\n" + "="*60)
    print("STATIC QUANTIZATION (INT8)")
    print("="*60)

    if calibration_data_path is None:
        print("⚠ Static quantization requires calibration data (--calibration-data)")
        print("  Skipping static quantization...")
        return False

    try:
        from onnxruntime.quantization import quantize_static, CalibrationDataReader, QuantType
    except ImportError:
        print("Warning: onnxruntime not installed. Skipping quantization.")
        return False

    # This is a placeholder - user needs to implement CalibrationDataReader
    print("⚠ Static quantization requires custom CalibrationDataReader implementation")
    print("  See: https://onnxruntime.ai/docs/performance/model-optimizations/quantization.html")
    return False


def optimize_graph(input_path, output_path):
    """
    Apply ONNX graph optimizations
    """
    print("\n" + "="*60)
    print("GRAPH OPTIMIZATION")
    print("="*60)

    try:
        from onnxruntime import SessionOptions, GraphOptimizationLevel, InferenceSession
        import onnxruntime as ort
    except ImportError:
        print("Warning: onnxruntime not installed. Skipping graph optimization.")
        return False

    try:
        sess_options = SessionOptions()
        sess_options.graph_optimization_level = GraphOptimizationLevel.ORT_ENABLE_ALL
        sess_options.optimized_model_filepath = str(output_path)

        session = InferenceSession(str(input_path), sess_options, providers=['CPUExecutionProvider'])
        print("✓ Graph optimization successful")
        return True
    except Exception as e:
        print(f"⚠ Graph optimization error: {e}")
        return False


def optimize_model(input_path, args):
    """
    Main optimization pipeline
    """
    input_path = Path(input_path)

    if not input_path.exists():
        print(f"Error: Model file '{input_path}' not found")
        sys.exit(1)

    # Determine output path
    if args.output:
        output_path = Path(args.output)
    else:
        # Auto-generate output name
        suffix = []
        if args.simplify or args.all:
            suffix.append("simplified")
        if args.fp16 or args.all:
            suffix.append("fp16")
        if args.quantize == 'dynamic' or args.all:
            suffix.append("quantized")
        if args.optimize or args.all:
            suffix.append("optimized")

        suffix_str = "_" + "_".join(suffix) if suffix else "_optimized"
        output_path = input_path.parent / f"{input_path.stem}{suffix_str}.onnx"

    original_size = get_model_size_mb(input_path)

    print("\n" + "="*60)
    print("ONNX MODEL OPTIMIZATION")
    print("="*60)
    print(f"Input:  {input_path.name} ({original_size:.2f} MB)")
    print(f"Output: {output_path.name}")
    print("="*60)

    # Create temporary working path
    current_path = input_path
    temp_dir = input_path.parent / ".temp_optimize"
    temp_dir.mkdir(exist_ok=True)

    step = 0

    try:
        # Step 1: Simplification
        if args.simplify or args.all:
            step += 1
            temp_path = temp_dir / f"step{step}.onnx"
            if simplify_model(current_path, temp_path):
                current_path = temp_path
                size = get_model_size_mb(current_path)
                print(f"  → Size after simplification: {size:.2f} MB ({size/original_size*100:.1f}%)")

        # Step 2: FP16 conversion
        if args.fp16 or args.all:
            step += 1
            temp_path = temp_dir / f"step{step}.onnx"
            if convert_fp16(current_path, temp_path):
                current_path = temp_path
                size = get_model_size_mb(current_path)
                print(f"  → Size after FP16: {size:.2f} MB ({size/original_size*100:.1f}%)")

        # Step 3: Quantization
        if args.quantize == 'dynamic' or args.all:
            step += 1
            temp_path = temp_dir / f"step{step}.onnx"
            if quantize_dynamic(current_path, temp_path):
                current_path = temp_path
                size = get_model_size_mb(current_path)
                print(f"  → Size after quantization: {size:.2f} MB ({size/original_size*100:.1f}%)")

        elif args.quantize == 'static':
            step += 1
            temp_path = temp_dir / f"step{step}.onnx"
            if quantize_static(current_path, temp_path, args.calibration_data):
                current_path = temp_path
                size = get_model_size_mb(current_path)
                print(f"  → Size after static quantization: {size:.2f} MB ({size/original_size*100:.1f}%)")

        # Step 4: Graph optimization
        if args.optimize or args.all:
            step += 1
            temp_path = temp_dir / f"step{step}.onnx"
            if optimize_graph(current_path, temp_path):
                current_path = temp_path
                size = get_model_size_mb(current_path)
                print(f"  → Size after graph optimization: {size:.2f} MB ({size/original_size*100:.1f}%)")

        # Copy final result to output
        shutil.copy(current_path, output_path)

        # Cleanup temp directory
        shutil.rmtree(temp_dir)

        final_size = get_model_size_mb(output_path)
        reduction = ((original_size - final_size) / original_size) * 100

        print("\n" + "="*60)
        print("✓ OPTIMIZATION COMPLETE")
        print("="*60)
        print(f"Original size: {original_size:.2f} MB")
        print(f"Final size:    {final_size:.2f} MB")
        print(f"Reduction:     {reduction:.1f}% ({original_size - final_size:.2f} MB saved)")
        print(f"Saved to:      {output_path.absolute()}")
        print("="*60)

    except Exception as e:
        print(f"\n❌ Error during optimization: {e}")
        if temp_dir.exists():
            shutil.rmtree(temp_dir)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description='Optimize ONNX models for size and performance',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Optimization techniques:
  --simplify          Remove redundant nodes and constant folding
  --fp16              Convert FP32 to FP16 (~50% size reduction)
  --quantize dynamic  Dynamic INT8 quantization (best for most cases)
  --quantize static   Static INT8 quantization (requires calibration data)
  --optimize          Apply ONNX Runtime graph optimizations
  --all               Apply all optimizations (recommended)

Examples:
  %(prog)s model.onnx --simplify --fp16
  %(prog)s model.onnx --quantize dynamic -o model_small.onnx
  %(prog)s model.onnx --all
  %(prog)s YOLOv11n.onnx --fp16 --simplify

Dependencies:
  pip install onnx onnxsim onnxruntime onnxconverter-common
        """
    )

    parser.add_argument(
        'model',
        type=str,
        help='Path to input ONNX model'
    )

    parser.add_argument(
        '-o', '--output',
        type=str,
        help='Output path (default: auto-generated based on optimizations)'
    )

    parser.add_argument(
        '--simplify',
        action='store_true',
        help='Simplify model (removes redundant nodes)'
    )

    parser.add_argument(
        '--fp16',
        action='store_true',
        help='Convert to FP16 precision'
    )

    parser.add_argument(
        '--quantize',
        choices=['dynamic', 'static'],
        help='Apply quantization (INT8)'
    )

    parser.add_argument(
        '--optimize',
        action='store_true',
        help='Apply ONNX Runtime graph optimizations'
    )

    parser.add_argument(
        '--all',
        action='store_true',
        help='Apply all optimizations (simplify + fp16 + quantize + optimize)'
    )

    parser.add_argument(
        '--calibration-data',
        type=str,
        help='Path to calibration data for static quantization'
    )

    args = parser.parse_args()

    # Check if any optimization is selected
    if not any([args.simplify, args.fp16, args.quantize, args.optimize, args.all]):
        print("Error: No optimization selected. Use --help for options.")
        print("Quick start: Use --all for full optimization pipeline")
        sys.exit(1)

    optimize_model(args.model, args)


if __name__ == '__main__':
    main()
