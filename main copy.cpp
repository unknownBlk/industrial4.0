#define BLYNK_TEMPLATE_ID "TMPL31nUdlfkG"
#define BLYNK_TEMPLATE_NAME "retroFix"
#define BLYNK_AUTH_TOKEN "lwbxi57FcgnEMk6rF5vVlsGENcLlWP23"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>          // ✅ Add this
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Default WiFi for Wokwi
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// Create a WiFi client
WiFiClient client;               // ✅ Add this

// Objects
Adafruit_MPU6050 mpu;

// Variables
float temperature = 0;
float ax, ay, az;
int potValue = 0;
int buttonState = 0;
int ledState = 0;
int tempAlert = 0;
float accelMag = 0;
float normalizedPot = 0;

// Virtual Pin Assignments
#define V_TEMP        V1
#define V_ACCELMAG    V2
#define V_POT         V3
#define V_BUTTON      V4
#define V_LED_CTRL    V5
#define V_OVERRIDE    V6
#define V_TEMP_ALERT  V7
#define V_NORM_POT    V8

#define LED_PIN 2
#define BUTTON_PIN 4
#define POT_PIN 34

// Handle LED control from app
BLYNK_WRITE(V_LED_CTRL) {
  ledState = param.asInt();
  digitalWrite(LED_PIN, ledState);
  Serial.print("LED Control: ");
  Serial.println(ledState ? "ON" : "OFF");
}

// Handle override potentiometer from app
BLYNK_WRITE(V_OVERRIDE) {
  int overrideValue = param.asInt();
  analogWrite(LED_PIN, overrideValue / 16); // PWM simulation
  Serial.print("Override Pot Value: ");
  Serial.println(overrideValue);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Test WiFi connection
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  int wifiTimeout = 10000; // 10 seconds timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
    while (1);
  }

  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }

  // Blynk connection
  Serial.println("Attempting Blynk connection...");
  Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
  if (!Blynk.connect(10000)) {
    Serial.println("Failed to connect to Blynk — running offline mode.");
  } else {
    Serial.println("Connected to Blynk Cloud!");
  }

  Serial.println("retroFix (Wokwi Mode) Started...");
}

void loop() {
  if (Blynk.connected()) {
    Blynk.run();
  }

  // Read sensors
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  ax = a.acceleration.x;
  ay = a.acceleration.y;
  az = a.acceleration.z;
  potValue = analogRead(POT_PIN);
  buttonState = !digitalRead(BUTTON_PIN);
  temperature = temp.temperature;

  // Derived values
  accelMag = sqrt(ax * ax + ay * ay + az * az);
  normalizedPot = (potValue / 4095.0) * 100.0;
  tempAlert = (temperature > 35.0) ? 1 : 0;

  // Send to Blynk if connected
  if (Blynk.connected()) {
    Blynk.virtualWrite(V_TEMP, temperature);
    Blynk.virtualWrite(V_ACCELMAG, accelMag);
    Blynk.virtualWrite(V_POT, potValue);
    Blynk.virtualWrite(V_BUTTON, buttonState);
    Blynk.virtualWrite(V_TEMP_ALERT, tempAlert);
    Blynk.virtualWrite(V_NORM_POT, normalizedPot);
  }

  // Console output
  Serial.print("Temp: "); Serial.print(temperature); Serial.print(" °C | ");
  Serial.print("AccelMag: "); Serial.print(accelMag); Serial.print(" g | ");
  Serial.print("Pot: "); Serial.print(potValue); Serial.print(" | ");
  Serial.print("Button: "); Serial.print(buttonState); Serial.print(" | ");
  Serial.print("TempAlert: "); Serial.print(tempAlert); Serial.print(" | ");
  Serial.print("NormPot: "); Serial.print(normalizedPot); Serial.println(" %");

  delay(2000);
}
