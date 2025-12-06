# Secure IoT Node for OTP-Based Access and Tamper Detection

Dual-ESP32 based smart parcel locker that uses **OTP authentication**, **motion sensing**, and **tamper detection**, with **MQTT** and **Blynk IoT Cloud** for secure last-mile delivery.

---

## 🚀 What This Project Does

- Uses **two ESP32 boards**:
  - **End Node** – PIR motion detection near the locker, sends trigger over MQTT.
  - **Edge Node** – IR parcel detection, OTP generation & verification, relay + solenoid lock control, buzzer + LED alerts.
- Sends **OTP** to the user via **Blynk** for parcel retrieval.
- Detects **tamper conditions** like:
  - Multiple wrong OTP entries
  - Suspicious motion while locker is locked
- Works even if the internet drops – core logic runs locally, Blynk syncs when Wi-Fi is back.

---

## 🧱 Repo Structure

```text
.
├─ end-node/
│  └─ end_node.ino          # ESP32 + PIR, MQTT publisher
└─ edge-node/
   └─ edge_node.ino         # ESP32 + IR + keypad + relay + buzzer + Blynk
