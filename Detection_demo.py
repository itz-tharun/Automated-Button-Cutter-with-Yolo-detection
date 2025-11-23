import cv2
from ultralytics import YOLO
import sys
# Removed imports: csv, numpy, serial, time

# --- Note on Calibration Data ---
# The original calibration data and perspective transform logic has been removed.
# M_perspective = cv2.getPerspectiveTransform(...)
# def convert_camera_to_stepper_perspective(...):
#     pass

def run_detection_and_display():
    """
    Runs real-time object detection using YOLO and displays the results.
    All serial communication and coordinate transformation logic has been removed.
    """
    try:
        # Load the YOLO model.
        # NOTE: You must ensure this path points to your actual model file.
        # If running on a different machine, this path must be updated.
        model = YOLO(r"C:\Python313\my_model.pt") 
    except Exception as e:
        print(f"Error loading model: {e}")
        print("Please verify the path to your 'my_model.pt' file.")
        return

    # Initialize video capture. Use 0 for default webcam, or 1 if it's the second device.
    # The original script used 1, so we'll keep that.
    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("Error: Could not open webcam. Check the camera index (0, 1, etc.)")
        return

    print("Starting video feed for YOLO detection. Press 'q' to exit.")

    frame_count = 0
    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error: Failed to grab frame.")
            break

        # --- Detection ---
        results = model(frame, verbose=False) # Run YOLO inference
        
        # Plot the bounding boxes and labels onto the frame
        annotated_frame = results[0].plot()
        boxes = results[0].boxes

        # --- Simplified Output (Optional: Print Detected Items) ---
        # The complex logging and stepper movement logic has been stripped out.
        if boxes is not None and len(boxes) > 0:
            for box in boxes:
                cls_id = int(box.cls[0])
                class_name = model.names[cls_id]
                conf = float(box.conf[0])
                
                # Calculate center for simple tracking (removed stepper conversion)
                xmin, ymin, xmax, ymax = box.xyxy[0].tolist()
                center_x = (xmin + xmax) / 2
                center_y = (ymin + ymax) / 2
                
                # Print simplified detection info
                if frame_count % 30 == 0: # Only print every 30 frames to avoid flooding console
                    print(f"Frame {frame_count}: Detected {class_name} at ({int(center_x)}, {int(center_y)}) with {conf*100:.2f}% confidence.")


        # Display the continuous detection frames
        cv2.imshow("Simplified YOLO Detection", annotated_frame)

        # Break the loop if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
            
        frame_count += 1

    # --- Cleanup ---
    cap.release()
    cv2.destroyAllWindows()
    # Removed serial closing
    print("\nDetection stopped and resources released.")

if __name__ == "__main__":
    run_detection_and_display()
