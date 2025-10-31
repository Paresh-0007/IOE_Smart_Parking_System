#include <Servo.h>

// IR Sensors
#define IR1_PIN D1  
#define IR2_PIN D5  
#define IR3_PIN D3 

// LEDs
#define LED1_PIN D7
#define LED2_PIN D8

// Servo
#define SERVO_PIN D4
Servo myServo;
int servoPosition = 180; // neutral
int servoStep = 180;     // step for rotation

void setup() {
  Serial.begin(9600);

  pinMode(IR1_PIN, INPUT);
  pinMode(IR2_PIN, INPUT);
  pinMode(IR3_PIN, INPUT);

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(servoPosition);

  Serial.println("System Initialized: IR + Servo Control");
}

void loop() {
  // Read IR sensor states
  bool ir1_detected = !digitalRead(IR1_PIN) == HIGH;
  bool ir2_detected = !digitalRead(IR2_PIN) == HIGH;
  bool ir3_detected = digitalRead(IR3_PIN) == HIGH;

  // LED Indicators
  digitalWrite(LED1_PIN, ir1_detected ? HIGH : LOW);
  digitalWrite(LED2_PIN, ir2_detected ? HIGH : LOW);

  // Servo Control
  if (ir3_detected) {
    servoPosition = max(0, servoPosition - servoStep); // rotate anticlockwise
    myServo.write(servoPosition);
    Serial.println("Gate Open (IR3 detected)");
    delay(500);
  } 
  else if (ir1_detected || ir2_detected || (ir1_detected && ir2_detected)) {
    servoPosition = min(180, servoPosition + servoStep); // rotate clockwise
    myServo.write(servoPosition);
    Serial.println("Gate Close (IR1 or IR2 detected)");
    delay(500);
  }

  // Object Classification
  if (!ir1_detected && ir2_detected) {
    Serial.println("Detected: BIKE");
  } 
  else if (!ir1_detected && !ir2_detected) {
    Serial.println("Detected: CAR");
  }

  delay(200);
}