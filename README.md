# IoT-Based Smart Home Automation System

This project presents a smart home automation system built using NodeMCU ESP8266, designed to provide secure access control, environmental monitoring, and real-time alerts. Key features include OTP-based door unlocking via Telegram, temperature-driven fan control, object and motion detection, and system feedback using sensors and actuators. It is compact, modular, and suitable for real-world smart home applications.

---

## Features

- OTP-based lock/unlock system using Telegram Bot
- Temperature and humidity monitoring with automated fan control
- Object detection using ultrasonic sensor
- Motion sensing with PIR sensor and responsive device control
- Real-time alerts via Telegram notifications

---

## Components Used

| Component             | Description                                              |
|----------------------|----------------------------------------------------------|
| NodeMCU ESP8266      | Wi-Fi-enabled microcontroller for control and communication |
| DHT11 Sensor          | Measures temperature and humidity                        |
| Servo Motor           | Simulates physical lock/unlock rotation                  |
| Ultrasonic Sensor     | Detects object presence using distance                   |
| PIR Sensor            | Detects motion in surrounding area                       |
| Relay Module          | Switches AC/DC devices like fans                         |
| LED & Buzzer          | Provide visual and audio feedback                        |
| Telegram Bot API      | Interface for user control and alert delivery            |
| 5V Power Supply       | Powers the system components                             |

---

## How It Works

1. **Initialization**: Connects to Wi-Fi, initializes sensors, and establishes Telegram Bot link.
2. **Access Control**:
   - Sends OTP on user request via Telegram
   - Verifies OTP and unlocks servo motor-based lock
   - Invalid OTP triggers buzzer alert
3. **Environment Monitoring**:
   - Continuously monitors temperature and humidity
   - Activates fan relay above a temperature threshold
   - Detects nearby objects and updates LED indicator
4. **Motion Detection**:
   - Detects motion using PIR sensor
   - Controls a relay-connected device based on movement
5. **Notifications**:
   - Sends real-time Telegram alerts for all major events

---

## Advantages

- Low-cost and scalable IoT-based solution
- Remote monitoring and control using Telegram
- Secure OTP-based authentication
- Automated environmental response
- Modular design for future enhancements

---

## Report

[View Full Project Report (PDF)](docs/Project_Report.pdf)

---

## Author

**Aman Sharma**  
B.Tech – Industrial Internet of Things  
National Institute of Technology, Kurukshetra  
Email: aman.sharma.12439@gmail.com  
Location: India
