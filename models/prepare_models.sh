#!/bin/bash
# Copy pre-quantized TrOCR models from trocr_quantized/ to models/

echo "Copying TrOCR encoder..."
cp models/trocr_quantized/encoder_model_quantized.onnx models/trocr_encoder_model_quantized.onnx

echo "Copying TrOCR decoder..."
cp models/trocr_quantized/decoder_model_quantized.onnx models/trocr_decoder_model_quantized.onnx

echo "✓ TrOCR models ready"
ls -lh models/*.onnx 2>/dev/null || echo "No ONNX files found yet"
