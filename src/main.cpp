#define BLYNK_TEMPLATE_ID "TMPL31nUdlfkG"
#define BLYNK_TEMPLATE_NAME "retroFix"
#define BLYNK_AUTH_TOKEN "lwbxi57FcgnEMk6rF5vVlsGENcLlWP23"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ThingSpeak.h>

// ---------- WiFi ----------
char ssid[] = "Wokwi-GUEST";   // Default Wokwi WiFi
char pass[] = "";

// ---------- ThingSpeak ----------
unsigned long myChannelNumber = 3118142;
const char *myWriteAPIKey = "UUQJ4R3REDVZLPKN";
WiFiClient client;

// ---------- Objects ----------
Adafruit_MPU6050 mpu;

// ---------- Variables ----------
float temperature = 0;
float ax, ay, az;
int potValue = 0;
float simulatedVoltage = 0;   // <-- Added for 220–280V mapping
int buttonState = 0;
int ledState = 0;
int tempAlert = 0;
float accelMag = 0;
float normalizedPot = 0;

// ---------- Virtual Pins ----------
#define V_TEMP        V1
#define V_ACCELMAG    V2
#define V_POT         V3          // now represents simulated voltage (220–280V)
#define V_BUTTON      V4
#define V_LED_CTRL    V5
#define V_OVERRIDE    V6
#define V_TEMP_ALERT  V7
#define V_NORM_POT    V8

#define LED_PIN 2
#define BUTTON_PIN 4
#define POT_PIN 34

// ---------- Blynk LED control ----------
BLYNK_WRITE(V_LED_CTRL) {
  ledState = param.asInt();
  digitalWrite(LED_PIN, ledState);
  Serial.print("LED Control: ");
  Serial.println(ledState ? "ON" : "OFF");
}

// ---------- Override Pot ----------
BLYNK_WRITE(V_OVERRIDE) {
  int overrideValue = param.asInt();
  analogWrite(LED_PIN, overrideValue / 16); // Simulated PWM
  Serial.print("Override Pot Value: ");
  Serial.println(overrideValue);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  int wifiTimeout = 10000; // 10s timeout
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n⚠️ WiFi not connected! Running in simulation mode...");
  }

  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }

  ThingSpeak.begin(client);

  // ---------- Blynk ----------
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  } else {
    Serial.println("⚠️ Blynk offline mode (simulation).");
  }

  Serial.println("✅ retroFix system initialized.");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }

  // ---------- Sensor Readings ----------
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  ax = a.acceleration.x;
  ay = a.acceleration.y;
  az = a.acceleration.z;
  potValue = analogRead(POT_PIN);
  buttonState = !digitalRead(BUTTON_PIN);
  temperature = temp.temperature;

  accelMag = sqrt(ax * ax + ay * ay + az * az);
  normalizedPot = (potValue / 4095.0) * 100.0;
  tempAlert = (temperature > 35.0) ? 1 : 0;

  // ---------- NEW: Map Potentiometer to 220V–280V range ----------
  simulatedVoltage = 220.0 + (potValue / 4095.0) * (280.0 - 220.0);

  // ---------- Send to Blynk ----------
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.virtualWrite(V_TEMP, temperature);
    Blynk.virtualWrite(V_ACCELMAG, accelMag);
    Blynk.virtualWrite(V_POT, simulatedVoltage); // <-- Now sending voltage
    Blynk.virtualWrite(V_BUTTON, buttonState);
    Blynk.virtualWrite(V_TEMP_ALERT, tempAlert);
    Blynk.virtualWrite(V_NORM_POT, normalizedPot);
  }

  // ---------- ThingSpeak Update ----------
  int statusCode;
  if (WiFi.status() == WL_CONNECTED) {
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, ax);
    ThingSpeak.setField(3, ay);
    ThingSpeak.setField(4, az);
    ThingSpeak.setField(5, simulatedVoltage); // <-- Send mapped voltage

    statusCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (statusCode == 200) {
      Serial.println("✅ ThingSpeak update successful!");
    } else {
      Serial.print("⚠️ ThingSpeak update failed, code: ");
      Serial.println(statusCode);
    }
  } else {
    Serial.println("🟡 ThingSpeak simulated (no WiFi in Wokwi).");
  }

  // ---------- Serial Monitor ----------
  Serial.println("----- retroFix Data -----");
  Serial.print("Temp: "); Serial.print(temperature); Serial.print(" °C | ");
  Serial.print("Pot Raw: "); Serial.print(potValue); Serial.print(" | ");
  Serial.print("Mapped Voltage: "); Serial.print(simulatedVoltage); Serial.println(" V");
  Serial.print("Button: "); Serial.print(buttonState); Serial.println();
  Serial.print("Accel: X="); Serial.print(ax);
  Serial.print(" Y="); Serial.print(ay);
  Serial.print(" Z="); Serial.print(az);
  Serial.print(" | Mag="); Serial.print(accelMag);
  Serial.println(" g");
  Serial.print("TempAlert: "); Serial.print(tempAlert);
  Serial.print(" | NormPot: "); Serial.print(normalizedPot); Serial.println(" %");
  Serial.println("-------------------------");

  delay(5000);
}
