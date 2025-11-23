# Automated Laser Cutting System for Garment Components (Buttons and Zippers)


## Overview

This project describes an automated, vision-controlled laser cutting system designed for the precise identification and cutting of small garment components, such as buttons and zippers, from fabric.

The system integrates three core technologies:

1. **Computer Vision:** A custom-trained YOLO model provides real-time object detection.

2. **Coordinate Transformation:** A perspective transform is used to accurately map pixel locations from the camera's view onto the gantry's physical coordinate system.

3. **Hardware Control:** An Arduino board manages the X-Y stepper motors and laser module based on commands received from the Python application via serial communication.
<br>

## Hardware Architecture

The complete system requires an integrated setup of optical, computational, and mechanical components:

| Component | Role | Notes |
| :-------: | :------: | :-------: |
| X-Y Gantry System | Provides the motion platform for the laser. | Requires precise, low-friction linear guides for accuracy. |
| Stepper Motors | Drive the X (longitudinal) and Y (traverse) movement of the laser module. | NEMA 17 or similar motors are suitable for small-scale applications. |
| Stepper Motor Drivers | Interface the motors with the microcontroller, managing current and step pulses. | A4988 drivers was used. |
| Arduino Board | The microcontroller responsible for low-level, real-time motor and laser control. | Used to execute the G-code-like commands (M, H, C) sent from the PC. |
| Laser Module | The cutting tool, mounted on the gantry. | Must have a TTL/PWM input for power modulation controlled by the Arduino. |
| Webcam / Camera | Captures the live video feed of the cutting bed. | Positioned to provide a clear view of the work area for object detection. |
| PC / Host Machine | Runs the main Python application (Final_Button_cutter.py). | Responsible for executing the heavy computational tasks (YOLO inference and coordinate mapping). |

<br> 

## Key Technical Concepts

The fundamental challenge in this system is ensuring that the cutting tool (the laser) moves to the exact physical location of a detected object.

### 1. Vision and Detection

The Python application uses the Final_Button_cutter.py script to:

Continuously capture a video frame from the camera.

Run the pre-trained YOLO model (my_model.pt) to identify and locate target objects (e.g., buttons, zippers) within that frame.

### 2. Coordinate Mapping

Since the camera view is often distorted (perspective effect) and does not align perfectly with the X-Y gantry's movement axes, a conversion is mandatory.

Perspective Transformation: The script uses a pre-calculated Perspective Transformation Matrix derived from four known points. This matrix takes the 2D pixel coordinates (x, y) of the detected object's center and transforms them into 3D-accurate, linear stepper motor coordinates (X, Y) relative to the gantry's home position.

This mapping step is what guarantees cutting accuracy despite the camera's viewing angle.

### 3. Execution Control Loop

After detection and mapping, the system initiates the cutting sequence:

The Python script establishes a serial link with the Arduino.

<br> 

## Repository Structure

| File Name | Description | Purpose |
| :-------: | :------: | :-------: |
| Final_Button_cutter.py | Main Application Script | Contains the vision pipeline, perspective transformation logic, and serial communication to manage the full, real-time cutting sequence.|
| Arduino_laser_control.ino | Arduino Firmware (Sketch) | The embedded code that receives serial commands and translates them into physical movements for the stepper motors and laser activation. |
| Detection_demo.py | Vision Testing Script | A standalone Python script used purely for testing the camera and YOLO model detection capabilities. |
| my_model.pt | YOLO Trained Weights | The file containing the trained model weights for object detection. |
| .gitignore | Git Configuration | Excludes temporary and environment-specific files (like detections.csv) from version control. |
<br>

## Arduino Firmware Protocol (Arduino_laser_control.ino)

The firmware implements a serial communication protocol designed to receive simple, comma-separated instructions from the Python application:

| Command Format | Description |
| :-------: | :------: |
| M, X_POS, Y_POS | Move Command: Instructs the laser gantry to move to the specified X_POS and Y_POS in steps. |
| H | Home Command: Initiates the homing sequence to establish the physical (0, 0) reference point. |
| C | Cut Command: Activates the laser module for a predefined duration and power level to perform the cut. |

## Data & Licensing Attribution
The detection model (my_model.pt) was trained using an Open Source Dataset. In compliance with the license, proper attribution is required for any redistribution or use of the model.
<br> 

### Dataset Citation

| Field | Value |
| :-------: | :------: |
| Title | 20250217test Dataset |
| Author | CHENCHEN |
| Source URL | https://universe.roboflow.com/chenchen-h220u/20250217test |
| License | CC BY 4.0 (Creative Commons Attribution 4.0 International Public License) |

<br> 

## Output Demonstration
<br>
<div align="center">
    <video src="https://github.com/user-attachments/assets/0dcc58e3-bd11-4276-a4fc-4e15bd360147">
</div>
<br>

## Conclusion and Future Work

The Automated Laser Cutting System successfully demonstrates a powerful integration of modern computer vision (YOLO) with precision electromechanical control (Arduino and Gantry). The core innovation lies in the robust perspective transformation, which reliably bridges the gap between the camera's distorted view and the physical world of the cutting machine, enabling high cutting accuracy.

### Potential areas for future development include:

**1. Speed and Efficiency:** Optimizing the YOLO model for faster inference or implementing batch processing for multi-object detection to increase throughput.

**2. Adaptive Calibration:** Implementing an automated calibration routine to dynamically calculate the perspective transformation matrix, eliminating the need for manual setup when the camera or gantry position is slightly adjusted.

**3. Advanced Path Planning:** Incorporating logic to determine the most efficient cutting order (e.g., using a Traveling Salesperson Problem solver) when multiple objects are detected, minimizing travel time and maximizing production speed.

