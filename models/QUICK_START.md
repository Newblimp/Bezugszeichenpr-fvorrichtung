# Quick Start Guide - Model Optimization

## 🚀 One-Line Commands

### Convert YOLO to ONNX
```bash
python convert_yolo_to_onnx.py model.pt --simplify
```

### Optimize Existing ONNX (Recommended)
```bash
python optimize_onnx.py model.onnx --all
```

### FP16 Only (Safest Optimization)
```bash
python optimize_onnx.py model.onnx --fp16
```

---

## 📊 Optimization Cheat Sheet

```
┌─────────────────────────────────────────────────────────┐
│  OPTIMIZATION METHOD        SIZE↓    SPEED↑   ACCURACY  │
├─────────────────────────────────────────────────────────┤
│  --simplify                  ~10%     +5%     No loss    │
│  --fp16                      ~50%    +20%     Minimal    │
│  --quantize dynamic          ~75%    +40%     <2% loss   │
│  --all (recommended)         ~85%    +60%     <3% loss   │
└─────────────────────────────────────────────────────────┘
```

---

## 🎯 Choose Your Goal

### Goal: **Maximum Size Reduction**
```bash
python optimize_onnx.py YOLOv11n.onnx --all
```
**Result:** 10.6 MB → ~1.5 MB (86% smaller)

### Goal: **Keep Best Accuracy**
```bash
python optimize_onnx.py YOLOv11n.onnx --simplify
```
**Result:** 10.6 MB → ~9.5 MB (no accuracy loss)

### Goal: **Balance Size & Accuracy**
```bash
python optimize_onnx.py YOLOv11n.onnx --fp16 --simplify
```
**Result:** 10.6 MB → ~5.3 MB (minimal accuracy loss)

### Goal: **Fastest CPU Inference**
```bash
python optimize_onnx.py YOLOv11n.onnx --quantize dynamic
```
**Result:** Best for CPU deployment

---

## 🔧 Installation

**One command to install everything:**
```bash
pip install ultralytics onnx onnxsim onnxruntime onnxconverter-common
```

---

## ⚡ Complete Workflow

```bash
# 1. Convert YOLO model to ONNX
python convert_yolo_to_onnx.py best.pt --simplify

# 2. Optimize the ONNX model
python optimize_onnx.py best.onnx --fp16 -o best_optimized.onnx

# 3. Update CMake to use optimized model
# Edit: cmake/EmbedModelsUncompressed.cmake
#   Change: models/YOLOv11n.onnx → models/best_optimized.onnx

# 4. Rebuild C++ application
cd build && cmake --build . -j8
```

---

## 📁 File Locations

```
models/
├── convert_yolo_to_onnx.py    ← Convert .pt to .onnx
├── optimize_onnx.py           ← Optimize .onnx files
├── README.md                  ← Full documentation
└── QUICK_START.md             ← This file
```

---

## ⚠️ Troubleshooting

**Error: "module not found"**
```bash
pip install ultralytics onnx onnxsim onnxruntime onnxconverter-common
```

**Model too large after optimization**
- Use `--all` flag for maximum compression
- Consider switching to smaller base model (e.g., YOLOv11n)

**Accuracy dropped too much**
- Use only `--fp16` (safest)
- Avoid quantization if accuracy is critical
- Test with `--simplify` only (no accuracy loss)

---

## 📈 Expected Results

### YOLOv11n (10.6 MB)
```
Original         → 10.6 MB
+ Simplify       →  9.5 MB  (10% smaller)
+ FP16           →  5.3 MB  (50% smaller)
+ Quantize       →  2.7 MB  (75% smaller)
+ All together   →  1.5 MB  (86% smaller)
```

### TrOCR Models
```
Encoder (23 MB)  → ~11 MB with --fp16
Decoder (40 MB)  → ~20 MB with --fp16
```

---

## 💡 Pro Tips

1. **Always test after optimization** - Run inference to verify accuracy
2. **Start with FP16** - Safest 50% reduction
3. **Use --all for deployment** - Maximum compression for production
4. **Keep original** - Script creates new file, original stays safe
5. **GPU vs CPU** - FP16 for GPU, Quantization for CPU

---

## 📞 Quick Help

```bash
# Show all options
python optimize_onnx.py --help
python convert_yolo_to_onnx.py --help

# See full docs
cat README.md
```

---

**Ready to optimize? Start here:**
```bash
python optimize_onnx.py YOLOv11n.onnx --all
```
