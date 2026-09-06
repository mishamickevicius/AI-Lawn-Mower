import time
import cv2
import numpy as np
import torch
from ultralytics import YOLO
from picamera2 import Picamera2

# 1. Initialize MIPI CSI-2 Camera via Picamera2
print("Initializing CSI-2 camera...")
picam2 = Picamera2()
config = picam2.create_preview_configuration(
    main={"format": "RGB888", "size": (640, 640)}
)
picam2.configure(config)
picam2.start()

# Warm up auto-exposure and white balance
time.sleep(1.0)

# Capture directly into a NumPy array (RGB format, shape: 640x640x3)
frame_rgb = picam2.capture_array()
picam2.stop()

print(f"Captured frame shape: {frame_rgb.shape}")

model_name = "yolov8n.pt"
print(f"\nBenchmarking {model_name} on Raspberry Pi 4 CPU...\n")

# ==========================================
# METHOD 1: ULTRALYTICS PIPELINE
# ==========================================
print("--- Method 1: Ultralytics Wrapper ---")
model_ul = YOLO(model_name)

# Warmup run
_ = model_ul(frame_rgb, verbose=False)

start_time = time.time()
results = model_ul(frame_rgb, verbose=False)
ul_latency = time.time() - start_time

print(f"Inference + Pre/Post-Processing Time: {ul_latency:.4f} seconds ({1/ul_latency:.2f} FPS)")
print(f"Objects detected: {len(results[0].boxes)}")

# ==========================================
# METHOD 2: RAW PYTORCH INFERENCE
# ==========================================
print("\n--- Method 2: Raw PyTorch Forward Pass ---")
ckpt = torch.load(model_name, map_location="cpu", weights_only=False)
model_pt = ckpt["model"].float().eval()

# Manual Preprocessing (HWC RGB uint8 -> NCHW float32 [0.0, 1.0])
img_chw = np.transpose(frame_rgb, (2, 0, 1))
img_tensor = torch.from_numpy(img_chw).float() / 255.0
img_tensor = img_tensor.unsqueeze(0)  # Shape: [1, 3, 640, 640]

# Warmup run
with torch.no_grad():
    _ = model_pt(img_tensor)

start_time = time.time()
with torch.no_grad():
    raw_preds = model_pt(img_tensor)
pt_latency = time.time() - start_time

print(f"Pure Forward-Pass Time: {pt_latency:.4f} seconds ({1/pt_latency:.2f} FPS)")
print(f"Raw Output Tensor Shape: {raw_preds[0].shape}")

# ==========================================
# OVERHEAD COMPARISON
# ==========================================
print("\n--- Summary ---")
overhead_ms = (ul_latency - pt_latency) * 1000
print(f"Ultralytics Wrapper Overhead (NMS + Letterbox formatting): {overhead_ms:.2f} ms")