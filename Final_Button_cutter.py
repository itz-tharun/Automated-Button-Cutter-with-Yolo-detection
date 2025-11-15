
import cv2
import csv
import numpy as np
from ultralytics import YOLO
import serial
import time
import sys

# --- Calibration Data ---
# The perspective transform requires 4 distinct points.
camera_points = np.float32([
    (87, 33), (533, 19), (109, 464), (541, 452)
])

stepper_points = np.float32([
    [0, 0],          # pt 1 for stepper
    [-2850, 0],      # pt 2 for stepper
    [0, -2750],      # pt 3 for stepper
    [-2850, -2750]   # pt 4 for stepper
])

# Use Perspective Transform for a more accurate mapping
try:
    M_perspective = cv2.getPerspectiveTransform(camera_points, stepper_points)
    print("Perspective calibration matrix (M_perspective) calculated successfully.")
    print("Transformation Matrix:")
    print(M_perspective)
except cv2.error as e:
    print(f"Error calculating transformation matrix: {e}")
    M_perspective = None

def convert_camera_to_stepper_perspective(x_camera, y_camera, M_perspective):
    """
    Converts a single set of camera coordinates to stepper coordinates using the perspective transform matrix.
    """
    if M_perspective is None:
        print("Error: Perspective transformation matrix not available.")
        return None

    # Reshape the coordinates for cv2.perspectiveTransform
    point = np.float32([[x_camera, y_camera]])
    point = np.array([point])

    # Apply the perspective transformation
    stepper_coords = cv2.perspectiveTransform(point, M_perspective)

    # Extract the transformed coordinates
    x_stepper = int(stepper_coords[0][0][0])
    y_stepper = int(stepper_coords[0][0][1])

    return x_stepper, y_stepper

def run_detection_and_log():
    try:
        # Replace 'COM8' with your Arduino's serial port
        ser = serial.Serial('COM8', 9600, timeout=1)
        time.sleep(2)
        print("Serial connection established with Arduino.")
    except serial.SerialException as e:
        print(f"Error: Could not open serial port: {e}")
        return

    model = YOLO(r"C:\Python313\my_model.pt")
    cap = cv2.VideoCapture(1)

    if not cap.isOpened():
        print("Error: Could not open webcam.")
        ser.close()
        return

    with open("detections.csv", mode="w", newline="") as csv_file:
        fieldnames = ["frame", "class", "confidence", "xmin", "ymin", "xmax", "ymax", "stepper_x", "stepper_y"]
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()

        frame_count = 0
        while True:
            ret, frame = cap.read()
            if not ret:
                print("Error: Failed to grab frame.")
                break

            results = model(frame)
            annotated_frame = results[0].plot()
            boxes = results[0].boxes

            if boxes is not None:
                for i, box in enumerate(boxes):
                    cls_id = int(box.cls[0])
                    class_name = model.names[cls_id]

                    if class_name == 'button':
                        conf = float(box.conf[0])
                        xmin, ymin, xmax, ymax = box.xyxy[0].tolist()
                        center_x = (xmin + xmax) / 2
                        center_y = (ymin + ymax) / 2
                        
                        stepper_coords = convert_camera_to_stepper_perspective(center_x, center_y, M_perspective)

                        if stepper_coords:
                            stepper_x, stepper_y = stepper_coords
                            
                            STEPS_PER_CM_X = 2850 / 28.5
                            STEPS_PER_CM_Y = 2750 / 27.5
                            SQUARE_SIZE_CM = 3
                            OFFSET_X = int(SQUARE_SIZE_CM / 2 * STEPS_PER_CM_X)
                            OFFSET_Y = int(SQUARE_SIZE_CM / 2 * STEPS_PER_CM_Y)
                            
                            square_points = [
                                (stepper_x - OFFSET_X, stepper_y - OFFSET_Y), # Top-Left
                                (stepper_x + OFFSET_X, stepper_y - OFFSET_Y), # Top-Right
                                (stepper_x + OFFSET_X, stepper_y + OFFSET_Y), # Bottom-Right
                                (stepper_x - OFFSET_X, stepper_y + OFFSET_Y), # Bottom-Left
                            ]
                            
                            print(f"\n--- Button Detected in Frame {frame_count} ---")
                            print(f"  Center Camera Coords: ({round(center_x, 2)}, {round(center_y, 2)})")
                            print(f"  Stepper Motor Centroid Coords: ({stepper_x}, {stepper_y})")
                            print(f"  Calculated Square Corners (stepper steps):")
                            
                            # Log all data to CSV
                            writer.writerow({
                                "frame": frame_count,
                                "class": class_name,
                                "confidence": round(conf, 4),
                                "xmin": round(xmin, 2),
                                "ymin": round(ymin, 2),
                                "xmax": round(xmax, 2),
                                "ymax": round(ymax, 2),
                                "stepper_x": stepper_x,
                                "stepper_y": stepper_y
                            })
                            
                            # Display the final frame with the button detected
                            cv2.imshow("Button & Zipper Detection", annotated_frame)
                            cv2.waitKey(1)  # Refresh the window
                            
                            try:
                                print("  ✅ Sending square commands...")
                                # Send the first command with a 6-second delay
                                first_point = square_points[0]
                                command_to_send = f"{first_point[0]},{first_point[1]}\n"
                                ser.write(command_to_send.encode('utf-8'))
                                print(f"    -> Sent command: {command_to_send.strip()}")
                                time.sleep(6)  # 6-second initial delay
                                
                                # Send the remaining commands with a 3-second delay
                                for point in square_points[1:]:
                                    command_to_send = f"{point[0]},{point[1]}\n"
                                    ser.write(command_to_send.encode('utf-8'))
                                    print(f"    -> Sent command: {command_to_send.strip()}")
                                    time.sleep(1)  # 3-second delay for subsequent moves
                                
                                # Send final "go home" command
                                home_command = "0,0\n"
                                ser.write(home_command.encode('utf-8'))
                                print("  -> Sent final command to return home.")
                                time.sleep(1)

                            except serial.SerialException as e:
                                print(f"Error writing to serial port: {e}")
                            
                            # Keep the annotated frame on screen for a moment
                            print("Press any key to close the window...")
                            cv2.waitKey(0)
                            
                            cap.release()
                            cv2.destroyAllWindows()
                            ser.close()
                            return # Exit the function

            # Display continuous detection frames
            cv2.imshow("Button & Zipper Detection", annotated_frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
            frame_count += 1

    cap.release()
    cv2.destroyAllWindows()
    ser.close()
    print("Serial connection closed.")

if __name__ == "__main__":
    run_detection_and_log()
