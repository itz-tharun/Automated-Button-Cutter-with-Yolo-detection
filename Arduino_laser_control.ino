#include <AccelStepper.h>


// ── X Axis (Motor 1) ─────────────────
#define X_STEP_PIN 2
#define X_DIR_PIN  3
#define X_EN_PIN   8


// ── Y Axis (Motor 2) ─────────────────
#define Y_STEP_PIN 4
#define Y_DIR_PIN  5
#define Y_EN_PIN   8


AccelStepper stepperX(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper stepperY(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);


const long REV_STEPS = 200;   // Adjust according to your microstep settings
bool motorsEnabled = true;    // Motor enable status
bool waitingForCoordinates = true;


// ── Direction Auto-Fix Flags ──
bool xDirInverted = false;
bool yDirInverted = false;


// ── Custom Origin System ──
long originX = 0;  // Origin offset for X axis
long originY = 0;  // Origin offset for Y axis


void setup() {
   Serial.begin(9600);


   // Motor setup
   stepperX.setEnablePin(X_EN_PIN);
   stepperY.setEnablePin(Y_EN_PIN);
   stepperX.setPinsInverted(false, false, true);
   stepperY.setPinsInverted(false, false, true);
   stepperX.enableOutputs();
   stepperY.enableOutputs();


   // Speed settings
   stepperX.setMaxSpeed(1000);
   stepperX.setAcceleration(500);
   stepperY.setMaxSpeed(1000);
   stepperY.setAcceleration(500);


   Serial.println(F("=== Absolute Coordinate Stepper Motor Control with Custom Origin ==="));
   Serial.println(F("System ready and waiting for absolute coordinates..."));
   printInstructions();
   printCurrentPosition();
}


void loop() {
   // Continuously run motors
   stepperX.run();
   stepperY.run();


   // Handle coordinate input
   if (Serial.available() > 0) {
       handleCoordinateInput();
   }


   // Display movement status and completion
   static unsigned long lastUpdate = 0;
   static bool wasMoving = false;
   bool isMoving = (stepperX.distanceToGo() != 0 || stepperY.distanceToGo() != 0);


   if (isMoving && millis() - lastUpdate > 1000) {
       Serial.print(F("Moving... Current: X:"));
       Serial.print(getRelativePositionX());
       Serial.print(F(" Y:"));
       Serial.print(getRelativePositionY());
       Serial.print(F(" | Remaining: X:"));
       Serial.print(stepperX.distanceToGo());
       Serial.print(F(" Y:"));
       Serial.println(stepperY.distanceToGo());
       lastUpdate = millis();
       wasMoving = true;
   }


   // When movement completes
   if (wasMoving && !isMoving) {
       Serial.println(F("✓ Movement completed!"));
       printCurrentPosition();
       Serial.println(F("Ready for next absolute coordinate..."));
       Serial.println();
       wasMoving = false;
   }
}


void handleCoordinateInput() {
   String input = Serial.readStringUntil('\n');
   input.trim();
   input.toLowerCase();


   // Handle absolute coordinate input (x,y format)
   int commaIndex = input.indexOf(',');
   if (commaIndex > 0) {
       long targetX = input.substring(0, commaIndex).toInt();
       long targetY = input.substring(commaIndex + 1).toInt();
       moveToAbsolute(targetX, targetY);
   }


   // Single axis absolute positioning
   else if (input.startsWith("x")) {
       long targetX = input.substring(1).toInt();
       long currentY = getRelativePositionY();
       moveToAbsolute(targetX, currentY);
   }
   else if (input.startsWith("y")) {
       long targetY = input.substring(1).toInt();
       long currentX = getRelativePositionX();
       moveToAbsolute(currentX, targetY);
   }
  
   // Control commands
   else if (input == "stop" || input == "s") {
       stopMotors();
   }
   else if (input == "home" || input == "reset") {
       resetToHome();
   }
   else if (input == "origin" || input == "setorigin") {
       setCurrentAsOrigin();
   }
   else if (input == "clearorigin") {
       clearOrigin();
   }
   else if (input == "gotoorigin" || input == "0,0") {
       moveToAbsolute(0, 0);
   }
   else if (input == "position" || input == "pos") {
       printCurrentPosition();
   }
   else if (input == "enable") {
       enableMotors();
   }
   else if (input == "disable") {
       disableMotors();
   }
   else if (input == "help" || input == "h") {
       printInstructions();
   }
   else if (input.startsWith("speed ")) {
       long speed = input.substring(6).toInt();
       if (speed >= 10 && speed <= 2000) {
           stepperX.setMaxSpeed(speed);
           stepperY.setMaxSpeed(speed);
           Serial.print(F("Speed set to: "));
           Serial.println(speed);
       } else {
           Serial.println(F("Error: Speed range 10-2000"));
       }
   }
   else if (input.startsWith("accel ")) {
       long accel = input.substring(6).toInt();
       if (accel >= 10 && accel <= 1000) {
           stepperX.setAcceleration(accel);
           stepperY.setAcceleration(accel);
           Serial.print(F("Acceleration set to: "));
           Serial.println(accel);
       } else {
           Serial.println(F("Error: Acceleration range 10-1000"));
       }
   }
   else if (input == "status") {
       printStatus();
   }
   else if (input == "testx") {
       testXMotor();
   }
   else if (input == "testy") {
       testYMotor();
   }
   else if (input == "pins") {
       printPinConfiguration();
   }
   else if (input != "") {
       Serial.println(F("Invalid format. Use: x,y for absolute coordinates"));
       Serial.println(F("Examples: 100,50 or -25,75 or x100 or y-30"));
       Serial.println(F("Type 'help' for all commands"));
   }
}


void moveToAbsolute(long targetX, long targetY) {
   if (motorsEnabled) {
       long currentRelX = getRelativePositionX();
       long currentRelY = getRelativePositionY();
      
       // Calculate absolute positions (relative to origin)
       long targetAbsX = originX + targetX;
       long targetAbsY = originY + targetY;


       Serial.print(F("Moving to absolute coordinates: ("));
       Serial.print(targetX);
       Serial.print(F(", "));
       Serial.print(targetY);
       Serial.print(F(") from origin"));
       Serial.println();


       Serial.print(F("Current position: ("));
       Serial.print(currentRelX);
       Serial.print(F(", "));
       Serial.print(currentRelY);
       Serial.print(F(") -> Target: ("));
       Serial.print(targetX);
       Serial.print(F(", "));
       Serial.print(targetY);
       Serial.print(F(")"));
       Serial.println();


       stepperX.moveTo(targetAbsX);
       stepperY.moveTo(targetAbsY);


       // Calculate movement distances for direction detection
       long moveX = targetX - currentRelX;
       long moveY = targetY - currentRelY;


       // Auto-detect wrong direction for X (only if moving)
       if (moveX != 0 && moveX < 0 && !xDirInverted) {
           delay(50);
           stepperX.run();
           if (stepperX.currentPosition() >= stepperX.targetPosition()) {
               Serial.println(F("⚠ Auto-correcting X direction..."));
               stepperX.setPinsInverted(false, true, true);
               xDirInverted = true;
           }
       }


       // Auto-detect wrong direction for Y (only if moving)
       if (moveY != 0 && moveY < 0 && !yDirInverted) {
           delay(50);
           stepperY.run();
           if (stepperY.currentPosition() >= stepperY.targetPosition()) {
               Serial.println(F("⚠ Auto-correcting Y direction..."));
               stepperY.setPinsInverted(false, true, true);
               yDirInverted = true;
           }
       }
      
       Serial.print(F("Distance to travel: X="));
       Serial.print(abs(moveX));
       Serial.print(F(" steps, Y="));
       Serial.print(abs(moveY));
       Serial.println(F(" steps"));
   } else {
       Serial.println(F("Motors disabled! Use 'enable' command first."));
   }
}






long getRelativePositionX() {
   return stepperX.currentPosition() - originX;
}


long getRelativePositionY() {
   return stepperY.currentPosition() - originY;
}


void setCurrentAsOrigin() {
   originX = stepperX.currentPosition();
   originY = stepperY.currentPosition();
   Serial.println(F("🎯 Current position set as new origin (0,0)"));
   Serial.print(F("Origin set at absolute position: X="));
   Serial.print(originX);
   Serial.print(F(", Y="));
   Serial.println(originY);
   printCurrentPosition();
}


void clearOrigin() {
   originX = 0;
   originY = 0;
   Serial.println(F("🔄 Origin cleared - now using absolute coordinates"));
   printCurrentPosition();
}


void testXMotor() {
   Serial.println(F("Testing X Motor - moving to absolute positions..."));
   long currentY = getRelativePositionY();
   moveToAbsolute(200, currentY);
   delay(3000);
   moveToAbsolute(0, currentY);
}


void testYMotor() {
   Serial.println(F("Testing Y Motor - moving to absolute positions..."));
   long currentX = getRelativePositionX();
   moveToAbsolute(currentX, 200);
   delay(3000);
   moveToAbsolute(currentX, 0);
}


void printPinConfiguration() {
   Serial.println(F("\n=== Pin Configuration ==="));
   Serial.println(F("X-Axis Motor:"));
   Serial.print(F("  Step Pin: "));
   Serial.println(X_STEP_PIN);
   Serial.print(F("  Dir Pin:  "));
   Serial.println(X_DIR_PIN);
   Serial.println(F("Y-Axis Motor:"));
   Serial.print(F("  Step Pin: "));
   Serial.println(Y_STEP_PIN);
   Serial.print(F("  Dir Pin:  "));
   Serial.println(Y_DIR_PIN);
   Serial.print(F("Enable Pin (shared): "));
   Serial.println(X_EN_PIN);
   Serial.println(F("========================\n"));
}


void stopMotors() {
   stepperX.stop();
   stepperY.stop();
   Serial.println(F("🛑 Motors stopped"));
   Serial.print(F("Stopped at position: "));
   printCurrentPosition();
}


void resetToHome() {
   Serial.println(F("Resetting stepper position to home (0,0) - this clears origin..."));
   stepperX.setCurrentPosition(0);
   stepperY.setCurrentPosition(0);
   originX = 0;
   originY = 0;
   Serial.println(F("✓ Position reset to home and origin cleared"));
   printCurrentPosition();
}


void enableMotors() {
   stepperX.enableOutputs();
   stepperY.enableOutputs();
   motorsEnabled = true;
   Serial.println(F("✓ Motors enabled"));
}


void disableMotors() {
   stepperX.disableOutputs();
   stepperY.disableOutputs();
   motorsEnabled = false;
   Serial.println(F("⚠ Motors disabled"));
}


void printCurrentPosition() {
   long relX = getRelativePositionX();
   long relY = getRelativePositionY();
   Serial.print(F("Current position: X="));
   Serial.print(relX);
   Serial.print(F(", Y="));
   Serial.print(relY);
   Serial.print(F(" (relative to origin)"));
   Serial.println();
   if (originX != 0 || originY != 0) {
       Serial.print(F("Absolute position: X="));
       Serial.print(stepperX.currentPosition());
       Serial.print(F(", Y="));
       Serial.print(stepperY.currentPosition());
       Serial.print(F(" | Origin at: X="));
       Serial.print(originX);
       Serial.print(F(", Y="));
       Serial.print(originY);
       Serial.println();
   }
}


void printStatus() {
   Serial.println(F("\n=== System Status ==="));
   printPinConfiguration();
   printCurrentPosition();
   Serial.print(F("Motors: "));
   Serial.println(motorsEnabled ? F("Enabled") : F("Disabled"));
   Serial.print(F("Max speed: "));
   Serial.println(stepperX.maxSpeed());
   Serial.print(F("Acceleration: "));
   Serial.println(stepperX.acceleration());
   if (stepperX.distanceToGo() != 0 || stepperY.distanceToGo() != 0) {
       long targetRelX = stepperX.targetPosition() - originX;
       long targetRelY = stepperY.targetPosition() - originY;
       Serial.print(F("Target position: X="));
       Serial.print(targetRelX);
       Serial.print(F(", Y="));
       Serial.print(targetRelY);
       Serial.print(F(" (relative to origin)"));
       Serial.println();
       Serial.print(F("Steps remaining: X="));
       Serial.print(stepperX.distanceToGo());
       Serial.print(F(", Y="));
       Serial.println(stepperY.distanceToGo());
   } else {
       Serial.println(F("Status: Ready for absolute coordinates"));
   }
   Serial.println();
}


void printInstructions() {
   Serial.println(F("\n=== Absolute Coordinate Control with Custom Origin ==="));
   Serial.println(F(" 🎯 ORIGIN SYSTEM:"));
   Serial.println(F("  origin      - Set current position as new origin (0,0)"));
   Serial.println(F("  clearorigin - Reset to absolute coordinates"));
   Serial.println(F("  gotoorigin  - Move to origin position (0,0)"));
   Serial.println(F("  💡 All coordinates are absolute from your custom origin!"));
   Serial.println();
   Serial.println(F(" 📐 ABSOLUTE COORDINATE INPUT:"));
   Serial.println(F("  Format: x,y (moves TO absolute position from origin)"));
   Serial.println(F("  Examples:"));
   Serial.println(F("    100,50    - Move TO position (100,50) from origin"));
   Serial.println(F("    -75,200   - Move TO position (-75,200) from origin"));
   Serial.println(F("    0,0       - Move TO origin position"));
   Serial.println(F("    50,-25    - Move TO position (50,-25) from origin"));
   Serial.println();
   Serial.println(F(" 🎯 SINGLE AXIS ABSOLUTE:"));
   Serial.println(F("  x100      - Move TO X=100 (keep current Y)"));
   Serial.println(F("  y-50      - Move TO Y=-50 (keep current X)"));
   Serial.println(F("  x0        - Move TO X=0 (origin X position)"));
   Serial.println();
   Serial.println(F(" 🛠 CONTROL COMMANDS:"));
   Serial.println(F("  stop      - Stop all movement"));
   Serial.println(F("  home      - Reset position counter to (0,0) and clear origin"));
   Serial.println(F("  position  - Show current position"));
   Serial.println(F("  status    - Show detailed status"));
   Serial.println(F("  enable    - Enable motors"));
   Serial.println(F("  disable   - Disable motors"));
   Serial.println();
   Serial.println(F(" 🔧 TESTING COMMANDS:"));
   Serial.println(F("  testx     - Test X motor with absolute positioning"));
   Serial.println(F("  testy     - Test Y motor with absolute positioning"));
   Serial.println(F("  pins      - Show pin configuration"));
   Serial.println();
   Serial.println(F(" ⚙ SETTINGS:"));
   Serial.println(F("  speed 800 - Set motor speed"));
   Serial.println(F("  accel 400 - Set acceleration"));
   Serial.println();
   Serial.println(F("💡 WORKFLOW EXAMPLE:"));
   Serial.println(F("1. Move to your desired reference point: '500,300'"));
   Serial.println(F("2. Set it as origin: 'origin'"));
   Serial.println(F("3. Now '0,0' moves to that point, '10,20' moves to (10,20) from there"));
   Serial.println(F("4. '100,150' always goes to the same absolute position from origin"));
   Serial.println(F("5. Use 'gotoorigin' to return to your reference point"));
   Serial.println(F("=====================================================\n"));
}
