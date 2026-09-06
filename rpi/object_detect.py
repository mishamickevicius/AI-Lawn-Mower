import cv2
import time
import torch
import numpy as np
from ultralytics import YOLO

## TODO pip install ultralytics opencv-python torch torchvision numpy python3-opencv ## 

# 1. Initialize MIPI CSI-2 Camera via GStreamer
# Grabs frames using the libcamera stack and pipes to OpenCV
pipeline = (
    "libcamerasrc ! "
    "video/x-raw, width=640, height=640, framerate=30/1 ! "
    "videoconvert ! "
    "appsink drop=true max-buffers=1"
)

cap = cv2.VideoCapture(pipeline, cv2.CAP_GSTREAMER)

# Warm up the camera sensor to allow auto-exposure/white balance to settle
for _ in range(10):
    cap.read()
    
ret, frame = cap.read()
cap.release()

if not ret:
    raise RuntimeError("Failed to capture image. Check camera connection or run with `libcamerify`.")

model_name = "yolov8n.pt"
print(f"Benchmarking {model_name} on Raspberry Pi CPU...\n")

# ==========================================
# METHOD 1: ULTRALYTICS PIPELINE
# ==========================================
print("--- Method 1: Ultralytics Wrapper ---")
# The YOLO class automatically downloads weights, handles letterbox preprocessing, and runs Non-Maximum Suppression.
model_ul = YOLO(model_name)

# Warmup inference (PyTorch builds the computational graph on the first pass)
_ = model_ul(frame, verbose=False)

start_time = time.time()
results = model_ul(frame, verbose=False)
ul_latency = time.time() - start_time

print(f"Inference + Pre/Post-Processing Time: {ul_latency:.4f} seconds")
print(f"Objects detected: {len(results[0].boxes)}")


# ==========================================
# METHOD 2: RAW PYTORCH INFERENCE
# ==========================================
print("\n--- Method 2: Raw PyTorch Neural Network ---")
# Extract the underlying PyTorch nn.Module from the downloaded checkpoint
ckpt = torch.load(model_name, map_location='cpu', weights_only=False)
model_pt = ckpt['model'].float().eval()

# Manual Preprocessing Pipeline
# YOLOv8 expects: RGB format, CHW layout, normalized float32 [0, 1], and a batch dimension.
# We use a naive 640x640 resize here. (Ultralytics uses dynamic letterboxing to preserve aspect ratio).
img_resized = cv2.resize(frame, (640, 640))
img_rgb = cv2.cvtColor(img_resized, cv2.COLOR_BGR2RGB)
img_chw = np.transpose(img_rgb, (2, 0, 1))
img_tensor = torch.from_numpy(img_chw).float() / 255.0
img_tensor = img_tensor.unsqueeze(0) # [1, 3, 640, 640]

# Warmup inference
with torch.no_grad():
    _ = model_pt(img_tensor)

start_time = time.time()
with torch.no_grad():
    raw_preds = model_pt(img_tensor)
pt_latency = time.time() - start_time

print(f"Pure Forward-Pass Time: {pt_latency:.4f} seconds")
print(f"Raw Tensor Output Shape: {raw_preds[0].shape}")

# ==========================================
# OVERHEAD COMPARISON
# ==========================================
print("\n--- Results ---")
overhead = (ul_latency - pt_latency) * 1000
print(f"Ultralytics Wrapper Overhead: {overhead:.2f} ms")