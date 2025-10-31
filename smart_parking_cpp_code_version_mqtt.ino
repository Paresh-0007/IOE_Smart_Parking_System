#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

// ---------------- WIFI + MQTT CONFIG ----------------
const char* ssid = "your wifi ssid";
const char* password = "your wifi password";
const char* mqtt_server = "mqtt ip"; // i.e. IPv4 address of your laptop where mosquitto mqtt is installed 
                                     // Make sure your mqtt port 1883 is open for 0.0.0.0 by configuring firewall 
                                     // Laptop must be connected at the same wifi as esp8266mod
WiFiClient espClient;
PubSubClient client(espClient);

#define MQTT_TOPIC_OBJECT "parking/object"
#define MQTT_TOPIC_STATUS "parking/status"

// ---------------- PIN DEFINITIONS ----------------
#define IR1_PIN D1
#define IR2_PIN D5
#define IR3_PIN D3
#define LED1_PIN D7
#define LED2_PIN D8
#define SERVO_PIN D4

Servo myServo;
int servoPosition = 180;
int servoStep = 180;
unsigned long lastReconnectAttempt = 0;

// --- State memory (to detect changes) ---
bool last_ir1 = false;
bool last_ir2 = false;
bool last_ir3 = false;
String last_object = "";

// ---------------- WIFI + MQTT FUNCTIONS ----------------
void setup_wifi() {
  delay(10);
  Serial.begin(9600);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

boolean reconnect() {
  if (client.connect("ESP8266-Parking")) {
    Serial.println("MQTT connected");
    client.publish(MQTT_TOPIC_STATUS, "Device online");
  }
  return client.connected();
}

// ---------------- SETUP ----------------
void setup() {
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  pinMode(IR1_PIN, INPUT);
  pinMode(IR2_PIN, INPUT);
  pinMode(IR3_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(servoPosition);

  Serial.println("System Initialized: IR + Servo + MQTT");
}

// ---------------- MAIN LOOP ----------------
void loop() {
  // --- Keep WiFi + MQTT alive ---
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnect()) lastReconnectAttempt = 0;
    }
  } else {
    client.loop();
  }

  // --- Read IR sensors ---
  bool ir1_detected = digitalRead(IR1_PIN) == LOW;
  bool ir2_detected = digitalRead(IR2_PIN) == LOW;
  bool ir3_detected = digitalRead(IR3_PIN) == LOW;

  // --- LED Indicators ---
  digitalWrite(LED1_PIN, ir1_detected ? HIGH : LOW);
  digitalWrite(LED2_PIN, ir2_detected ? HIGH : LOW);

  // --- Gate Control ---
  if (ir3_detected && !last_ir3) {  // IR3 just detected
    servoPosition = 0;
    myServo.write(servoPosition);
    Serial.println("Gate Open (IR3 detected)");
    client.publish(MQTT_TOPIC_STATUS, "Gate Open");
  } else if (!ir3_detected && last_ir3) {  // IR3 just cleared
    servoPosition = 180;
    myServo.write(servoPosition);
    Serial.println("Gate Close (IR3 cleared)");
    client.publish(MQTT_TOPIC_STATUS, "Gate Close");
  }

  // --- Vehicle Type Detection (on change) ---
  String detected_object = "";

  if (ir1_detected && ir2_detected) {
    detected_object = "car";
  } else if (!ir1_detected && ir2_detected) {
    detected_object = "bike";
  } else {
    detected_object = "";
  }

  if (detected_object != last_object) {
    if (detected_object != "") {
      Serial.print("Detected: ");
      Serial.println(detected_object);
      client.publish(MQTT_TOPIC_OBJECT, detected_object.c_str());
    } else {
      Serial.println("No vehicle detected");
      client.publish(MQTT_TOPIC_OBJECT, "none");
    }
    last_object = detected_object;
  }

  // --- Update last sensor states ---
  last_ir1 = ir1_detected;
  last_ir2 = ir2_detected;
  last_ir3 = ir3_detected;

  delay(100); // smoother performance
}
