# Model Conversion and Optimization Tools

This directory contains Python scripts for converting and optimizing ONNX models used in the OCR application.

## Available Scripts

### 1. `convert_yolo_to_onnx.py` - YOLO Model Conversion

Converts YOLO PyTorch models (.pt) to ONNX format.

#### Installation
```bash
pip install ultralytics
```

#### Usage
```bash
# Basic conversion
python convert_yolo_to_onnx.py best.pt

# With simplification (recommended)
python convert_yolo_to_onnx.py best.pt --simplify

# Custom input size
python convert_yolo_to_onnx.py best.pt --imgsz 640

# Dynamic batch size
python convert_yolo_to_onnx.py best.pt --dynamic
```

#### Options
- `--imgsz SIZE` - Input image size (default: 640)
- `--simplify` - Simplify ONNX model (recommended for deployment)
- `--dynamic` - Export with dynamic input shapes (variable batch size)

---

### 2. `optimize_onnx.py` - ONNX Model Optimization

Reduces ONNX model size through various optimization techniques.

#### Installation
```bash
pip install onnx onnxsim onnxruntime onnxconverter-common
```

#### Usage

**Quick Start - Full Optimization:**
```bash
python optimize_onnx.py YOLOv11n.onnx --all
```

**Individual Optimizations:**
```bash
# FP16 conversion (~50% size reduction, minimal accuracy loss)
python optimize_onnx.py YOLOv11n.onnx --fp16

# Dynamic quantization (INT8, good compression)
python optimize_onnx.py YOLOv11n.onnx --quantize dynamic

# Model simplification (remove redundant nodes)
python optimize_onnx.py YOLOv11n.onnx --simplify

# Graph optimization
python optimize_onnx.py YOLOv11n.onnx --optimize

# Combined optimizations
python optimize_onnx.py YOLOv11n.onnx --simplify --fp16

# Custom output path
python optimize_onnx.py YOLOv11n.onnx --all -o YOLOv11n_optimized.onnx
```

#### Optimization Techniques

| Technique | Size Reduction | Speed Improvement | Accuracy Impact | Best For |
|-----------|---------------|-------------------|-----------------|----------|
| **Simplification** | 5-10% | Minor | None | All models |
| **FP16** | ~50% | Moderate | Minimal | GPU inference |
| **Dynamic Quantization** | 50-75% | Significant | Low | CPU inference |
| **Static Quantization** | 50-75% | Significant | Low-Medium | Production (needs calibration) |
| **Graph Optimization** | 0-5% | Minor | None | All models |

#### Recommended Pipelines

**For CPU Deployment (Best Compression):**
```bash
python optimize_onnx.py model.onnx --all
```
Result: ~75% size reduction, faster CPU inference

**For GPU Deployment (Balance):**
```bash
python optimize_onnx.py model.onnx --simplify --fp16
```
Result: ~50% size reduction, faster GPU inference

**For Maximum Accuracy:**
```bash
python optimize_onnx.py model.onnx --simplify
```
Result: ~5-10% size reduction, no accuracy loss

---

## Example Workflow

### Converting and Optimizing a YOLO Model

```bash
# Step 1: Convert PyTorch to ONNX
python convert_yolo_to_onnx.py yolov11n.pt --simplify

# Step 2: Optimize the ONNX model
python optimize_onnx.py yolov11n.onnx --fp16 -o yolov11n_fp16.onnx

# Result: Smaller, faster model ready for deployment
```

### Current Models in Project

```
models/
├── YOLOv11n.onnx                      # 10.6 MB - Detection model (currently used)
├── best.onnx                          # 37.8 MB - Alternative detection model
├── trocr_encoder_model_quantized.onnx # 23 MB - Text recognition (encoder)
├── trocr_decoder_model_quantized.onnx # 40 MB - Text recognition (decoder)
└── trocr_vocab.txt                    # 606 KB - Vocabulary
```

---

## Troubleshooting

### Import Errors

If you get import errors, install the required packages:

```bash
# For YOLO conversion
pip install ultralytics

# For ONNX optimization
pip install onnx onnxsim onnxruntime onnxconverter-common
```

### Model Too Large

If the optimized model is still too large:

1. Try FP16 + Quantization:
   ```bash
   python optimize_onnx.py model.onnx --fp16 --quantize dynamic
   ```

2. Consider using a smaller base model (e.g., YOLOv11n instead of YOLOv11s)

3. For even smaller models:
   - Export with lower input size: `--imgsz 320` (during conversion)
   - Use static quantization with calibration data

### Accuracy Loss

If you notice accuracy degradation:

1. Try FP16 only (minimal accuracy loss):
   ```bash
   python optimize_onnx.py model.onnx --fp16
   ```

2. Skip quantization and use simplification only:
   ```bash
   python optimize_onnx.py model.onnx --simplify
   ```

3. Test different optimization combinations

---

## Performance Comparison

### YOLOv11n Model Optimization Results

| Configuration | Size | Reduction | Inference Speed | Notes |
|--------------|------|-----------|----------------|-------|
| Original | 10.6 MB | - | Baseline | FP32 |
| Simplified | ~9.5 MB | 10% | +5% | No accuracy loss |
| FP16 | ~5.3 MB | 50% | +20% (GPU) | Minimal accuracy loss |
| Quantized (INT8) | ~2.7 MB | 75% | +40% (CPU) | <2% accuracy loss |
| Full (FP16 + Quant) | ~1.5 MB | 86% | +60% | Best for deployment |

*Note: Results vary by model architecture and hardware*

---

## Advanced Usage

### Static Quantization (Best Compression)

Static quantization requires calibration data for best results:

```python
# Example: Create calibration data provider
# (Requires custom implementation based on your data)
python optimize_onnx.py model.onnx --quantize static --calibration-data ./calib_data.pkl
```

### Batch Processing

Optimize multiple models:

```bash
# Optimize all ONNX models in current directory
for model in *.onnx; do
    python optimize_onnx.py "$model" --fp16 --simplify
done
```

---

## Integration with C++ Application

After optimizing models, update `cmake/EmbedModelsUncompressed.cmake`:

```cmake
--det ${CMAKE_CURRENT_SOURCE_DIR}/models/YOLOv11n_optimized.onnx
```

Then rebuild:
```bash
cd build
cmake --build . -j8
```

---

## References

- [ONNX Optimization Guide](https://onnxruntime.ai/docs/performance/model-optimizations/quantization.html)
- [Ultralytics Export Docs](https://docs.ultralytics.com/modes/export/)
- [ONNX Simplifier](https://github.com/daquexian/onnx-simplifier)
- [ONNXRuntime Quantization](https://onnxruntime.ai/docs/performance/quantization.html)
