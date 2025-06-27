#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Servo.h>

// WiFi & Telegram Credentials
const char* ssid = "SSID";
const char* password = "wifi_password";
const char* botToken = "Telegram_bot_token";
const int64_t chatId = 12345678;

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

#define DHTPIN 14  // D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Servo lockServo;
const int buzzerPin = 5;    // D1
const int fanLedPin = 2;    // D4
const int trigPin = 13;     // D7
const int echoPin = 12;     // D6
const int objectLedPin = 15;// D8
#define PIR_PIN D2          // GPIO4
#define RELAY_PIN D3        // GPIO0

String generatedOTP = "";
bool otpSent = false;
bool isLocked = true;
bool waitingForCommand = true;
int currentAction = -1;
bool fanState = false;
bool prevObjectLedState = LOW;
bool lastMotion = false;
unsigned long lastMotionTime = 0;
bool manualFanControl = false;
float lastTemp = 0;

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  pinMode(fanLedPin, OUTPUT);
  pinMode(objectLedPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(buzzerPin, LOW);
  digitalWrite(fanLedPin, LOW);
  digitalWrite(objectLedPin, LOW);
  digitalWrite(RELAY_PIN, HIGH);

  dht.begin();
  lockServo.attach(16);
  lockServo.write(0);

  WiFi.begin(ssid, password);
  client.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to Wi-Fi");
  Serial.println("System is LOCKED. Type 1 to UNLOCK.");
}

void loop() {
  handleSerialInput();
  handleOTPInput();

  if (!isLocked) {
    handleTemperature();
    handleMotionSensor();
    detectObject();
    delay(1000);
  }
}

void handleSerialInput() {
  if (waitingForCommand && Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "1" && isLocked) {
      currentAction = 1;
      sendOTP();
      waitingForCommand = false;
    } else if (input == "0" && !isLocked) {
      currentAction = 0;
      sendOTP();
      waitingForCommand = false;
    } else if (input == "on" && !isLocked) {
      digitalWrite(fanLedPin, HIGH);
      digitalWrite(RELAY_PIN, LOW);
      fanState = true;
      manualFanControl = true;
      Serial.println("Fan and light manually turned ON");
    } else if (input == "off" && !isLocked) {
      digitalWrite(fanLedPin, LOW);
      digitalWrite(RELAY_PIN, HIGH);
      fanState = false;
      manualFanControl = true;
      Serial.println("Fan and light manually turned OFF");
    } else if (input == "auto" && !isLocked) {
      manualFanControl = false;
      Serial.println("Fan and light set to AUTO mode");
    } else {
      Serial.println("Invalid input or action already done.");
    }

    showAvailableActions();
  }
}

void handleOTPInput() {
  if (otpSent && Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == generatedOTP) {
      Serial.println("OTP Verified");
      bot.sendMessage(String(chatId), "OTP Verified", "");
      shortBeepTwice();
      (currentAction == 1) ? unlockSystem() : lockSystem();
    } else {
      Serial.println("Wrong OTP");
      bot.sendMessage(String(chatId), "Wrong OTP", "");
      longBeep();
    }

    otpSent = false;
    waitingForCommand = true;
    showAvailableActions();
  }
}

void smoothServoMove(int startAngle, int endAngle) {
  // int step = (startAngle < endAngle) ? 1 : -1;
  // for (int pos = startAngle; pos != endAngle; pos += step) {
  //   lockServo.write(pos);
  //   delay(5);
  // }
  lockServo.write(endAngle);
}

void unlockSystem() {
  isLocked = false;
  smoothServoMove(0, 180);
  delay(100);

  Serial.println("Unlocked");
  bot.sendMessage(String(chatId), "System Unlocked", "");
  sendTemperatureHumidity();
}

void lockSystem() {
  isLocked = true;
  smoothServoMove(180, 0);
  delay(100);

  Serial.println("Locked");
  bot.sendMessage(String(chatId), "System Locked", "");

  digitalWrite(fanLedPin, LOW);
  digitalWrite(RELAY_PIN, HIGH);
  fanState = false;
}

long getAverageDistance() {
  long total = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    total += pulseIn(echoPin, HIGH);
    delay(10);
  }
  long avgDuration = total / 5;
  return (avgDuration * 0.0344) / 2;
}

void detectObject() {
  long distance = getAverageDistance();

  if (distance < 10 && prevObjectLedState) {
    digitalWrite(objectLedPin, LOW);
    prevObjectLedState = false;
    delay(500);
  } else if (distance > 15 && !prevObjectLedState) {
    digitalWrite(objectLedPin, HIGH);
    prevObjectLedState = true;
    delay(500);
  }
}

void handleMotionSensor() {
  bool motion = digitalRead(PIR_PIN);
  unsigned long currentMillis = millis();

  if (!manualFanControl && motion && !lastMotion && (currentMillis - lastMotionTime > 5000)) {
    lastMotionTime = currentMillis;
    fanState = !fanState;

    digitalWrite(fanLedPin, fanState ? HIGH : LOW);
    digitalWrite(RELAY_PIN, fanState ? LOW : HIGH);

    Serial.println(fanState ? "Motion Detected: Fan ON, Light ON" : "Motion Detected: Fan OFF, Light OFF");
    bot.sendMessage(String(chatId), fanState ? "Motion: Fan ON, Light ON" : "Motion: Fan OFF, Light OFF", "");

    sendTemperatureHumidity();
  }
  lastMotion = motion;
}

void handleTemperature() {
  if (!manualFanControl) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    if (t > 29 && !fanState) {
      digitalWrite(fanLedPin, HIGH);
      digitalWrite(RELAY_PIN, LOW);
      fanState = true;
      Serial.println("Auto Temp: Fan ON, Light ON");
      bot.sendMessage(String(chatId), "Auto Temp: Fan ON, Light ON", "");
    } else if (t < 28 && fanState) {
      digitalWrite(fanLedPin, LOW);
      digitalWrite(RELAY_PIN, HIGH);
      fanState = false;
      Serial.println("Auto Temp: Fan OFF, Light OFF");
      bot.sendMessage(String(chatId), "Auto Temp: Fan OFF, Light OFF", "");
    }

    if (abs(t - lastTemp) > 1.0) {
      sendTemperatureHumidity();
      lastTemp = t;
    }
  }
}

void sendOTP() {
  generatedOTP = String(random(1000, 9999));
  String msg = "OTP to " + String(currentAction == 1 ? "UNLOCK" : "LOCK") + ": " + generatedOTP;
  bot.sendMessage(String(chatId), msg, "");
  Serial.println("OTP sent. Enter the OTP:");
  otpSent = true;
}

void sendTemperatureHumidity() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print("°C  |  Humidity: ");
  Serial.print(h);
  Serial.println("%");

  String dhtMessage = " Temp: " + String(t) + "°C\n Humidity: " + String(h) + "%";
  bot.sendMessage(String(chatId), dhtMessage, "");
}

void showAvailableActions() {
  if (isLocked) {
    Serial.println("Type 1 to UNLOCK.");
  } else {
    Serial.println("Type 0 to LOCK. Type 'on'/'off'/'auto' to control fan/light.");
  }
}

void shortBeepTwice() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(150);
    digitalWrite(buzzerPin, LOW);
    delay(150);
  }
}

void longBeep() {
  digitalWrite(buzzerPin, HIGH);
  delay(2000);
  digitalWrite(buzzerPin, LOW);
}
