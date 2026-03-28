#include <Servo.h>

Servo myServo;
int currentPos = 90;
int targetPos = 90;
unsigned long lastMoveTime = 0;

void setup() {
  Serial.begin(115200);
  myServo.attach(9);
  myServo.write(90);
}

void loop() {
  // Read serial data (non-blocking)
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    // Parse "ax,ay,az,gx,gy,gz" — we only need az (3rd value)
    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    int c3 = line.indexOf(',', c2 + 1);
    if (c1 < 0 || c2 < 0) return;

    float az = line.substring(c2 + 1, c3 > 0 ? c3 : line.length()).toFloat();

    // Two states based on Z-axis:
    // az < 0 → sensor facing UP   → servo goes to 180 (push ball up)
    // az > 0 → sensor facing DOWN → servo goes to 0   (reverse, push ball up)
    if (az < -2.0) {
      targetPos = 180;
    } else if (az > 2.0) {
      targetPos = 0;
    }
  }

  // Gradual servo movement (mid speed)
  // Move 1 degree every 15ms ≈ 2.7 seconds for full sweep
  if (millis() - lastMoveTime >= 15) {
    lastMoveTime = millis();
    if (currentPos < targetPos) {
      currentPos++;
    } else if (currentPos > targetPos) {
      currentPos--;
    }
    myServo.write(currentPos);
  }
}
