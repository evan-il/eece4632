import cv2
import os

INPUT_VIDEO = "input.mp4"
OUTPUT_DIR  = "output_frames"

os.makedirs(OUTPUT_DIR, exist_ok=True)

cap = cv2.VideoCapture(INPUT_VIDEO)
if not cap.isOpened():
    raise RuntimeError(f"Could not open: {INPUT_VIDEO}")

total = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
i = 0
while True:
    ret, frame = cap.read()
    if not ret:
        break
    cv2.imwrite(os.path.join(OUTPUT_DIR, f"frame_{i:05d}.png"), frame)
    i += 1
    if i % 100 == 0:
        print(f"  {i}/{total}")

cap.release()
print(f"Done. {i} frames saved to '{OUTPUT_DIR}/'")
