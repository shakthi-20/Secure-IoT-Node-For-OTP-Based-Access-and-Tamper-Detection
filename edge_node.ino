#define BLYNK_TEMPLATE_ID "TMPL3x-ZkFg7w"
#define BLYNK_TEMPLATE_NAME "Smart Parcel Locker"
#define BLYNK_AUTH_TOKEN "4VZnnYojSgAOsm6EBRPBkqa8qidMkkT5"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <PubSubClient.h>
#include <Keypad.h>

// ===== WiFi + MQTT =====
const char* ssid = "POCO X6 5G";
const char* password = "12345678";
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);
BlynkTimer timer;

// ===== Hardware Pins =====
#define IR_PIN 35
#define RELAY_PIN 23
#define GREEN_LED 14
#define RED_LED 12
#define BUZZER_PIN 2

// ===== Keypad Setup =====
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26};
byte colPins[COLS] = {4, 21, 18, 19};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===== Variables =====
String generatedOTP = "";
String enteredOTP = "";
bool parcelInside = false;
int wrongAttempts = 0;

// ===== Helper Functions =====
String generateOTP() {
  String otp = "";
  for (int i = 0; i < 4; i++) otp += String(random(0, 9));
  return otp;
}

void updateLEDs(bool unlocked) {
  digitalWrite(GREEN_LED, unlocked);
  digitalWrite(RED_LED, !unlocked);
}

// ===== Lock/Unlock Locker =====
void lockBox(bool unlock) {
  // Active LOW relay: LOW = UNLOCK, HIGH = LOCK
  digitalWrite(RELAY_PIN, unlock ? LOW : HIGH);
  updateLEDs(unlock);
  Blynk.virtualWrite(V0, unlock ? "Unlocked 🔓" : "Locked 🔒");
  Serial.println(unlock ? "🔓 Locker UNLOCKED" : "🔒 Locker LOCKED");
  Blynk.virtualWrite(V4, unlock ? "🔓 Locker UNLOCKED" : "🔒 Locker LOCKED");
}

// ===== MQTT Callback =====
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];
  message.trim();

  Serial.print("📩 MQTT Message on "); Serial.print(topic); Serial.print(": "); Serial.println(message);
  Blynk.virtualWrite(V4, "📩 MQTT: " + message);

  if (String(topic) == "parcelLocker/pir" && message == "motion_detected") {
    if (!parcelInside) {
      Serial.println("👀 Motion detected → Unlocking locker...");
      tone(BUZZER_PIN, 2000, 200);
      lockBox(true);
      Blynk.virtualWrite(V4, "👀 Motion detected → Locker UNLOCKED for delivery");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("LockerEdgeNode")) {
      Serial.println("connected!");
      client.subscribe("parcelLocker/pir");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      delay(2000);
    }
  }
}

// ===== Check IR sensor & control relay =====
void checkParcel() {
  int irState = digitalRead(IR_PIN);

  if (irState == LOW) {
    // Parcel detected → Lock locker
    digitalWrite(RELAY_PIN, HIGH);
    updateLEDs(false);

    if (!parcelInside) {
      parcelInside = true;
      generatedOTP = generateOTP();

      Serial.print("📦 Parcel detected! OTP: "); Serial.println(generatedOTP);
      Blynk.virtualWrite(V4, "📦 Parcel detected! OTP: " + generatedOTP);

      Blynk.virtualWrite(V1, generatedOTP);
      Blynk.virtualWrite(V2, "Parcel Inside 📦");
      Blynk.virtualWrite(V3, 1);

      tone(BUZZER_PIN, 1500, 200);
      Blynk.logEvent("parcel_arrived", "Parcel placed. OTP: " + generatedOTP);
    }
  } 
  else {
    // No parcel → Unlock locker
    digitalWrite(RELAY_PIN, LOW);
    updateLEDs(true);

    if (parcelInside == false) {
      Blynk.virtualWrite(V2, "Locker Empty");
      Blynk.virtualWrite(V3, 0);
      Serial.println("✅ No object → Locker unlocked");
      Blynk.virtualWrite(V4, "✅ No object detected → Locker UNLOCKED");
    }
  }
}

// ===== Keypad handling =====
void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  // show each key press live
  Serial.print("Key Pressed: "); Serial.println(key);
  Blynk.virtualWrite(V4, "🔢 Key pressed: " + String(key));

  if (key == '#') {
    Blynk.virtualWrite(V4, "🔐 OTP Submitted: " + enteredOTP);

    if (enteredOTP == generatedOTP && parcelInside) {
      Serial.println("✅ Correct OTP! Unlocking locker...");
      Blynk.virtualWrite(V4, "✅ Correct OTP! Unlocking locker...");
      tone(BUZZER_PIN, 2000, 300);

      digitalWrite(RELAY_PIN, LOW); // Unlock
      updateLEDs(true);
      delay(5000); // pickup time
      digitalWrite(RELAY_PIN, HIGH); // Lock again
      updateLEDs(false);

      parcelInside = false;
      enteredOTP = "";
      wrongAttempts = 0;

      Blynk.virtualWrite(V2, "Locker Empty");
      Blynk.virtualWrite(V3, 0);
      Blynk.virtualWrite(V4, "✅ Locker opened & re-locked after pickup");
      Blynk.logEvent("locker_opened", "Locker opened successfully!");
    } 
    else {
      wrongAttempts++;
      Serial.println("❌ Wrong OTP!");
      tone(BUZZER_PIN, 1000, 400);
      Blynk.virtualWrite(V4, "❌ Wrong OTP entered!");

      if (wrongAttempts >= 3) {
        Blynk.logEvent("alert", "🚨 Multiple Wrong Attempts!");
        Serial.println("🚨 ALERT: Multiple Wrong Attempts!");
        Blynk.virtualWrite(V4, "🚨 ALERT: Multiple Wrong Attempts!");
        wrongAttempts = 0;
      }
      enteredOTP = "";
    }
  } 
  else if (key == '*') {
    enteredOTP = "";
    Serial.println("🔄 OTP Cleared");
    Blynk.virtualWrite(V4, "🔄 OTP Cleared");
  } 
  else {
    enteredOTP += key;
    Serial.print("Entered OTP so far: ");
    Serial.println(enteredOTP);
    Blynk.virtualWrite(V4, "🟩 Entered OTP: " + enteredOTP);
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW); // Start unlocked
  updateLEDs(true);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ WiFi Connected");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timer.setInterval(500L, checkParcel);

  Serial.println("🚀 Smart Parcel Locker Ready!");
  Blynk.virtualWrite(V4, "🚀 System Ready. Waiting for motion via MQTT...");
}

// ===== Loop =====
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  Blynk.run();
  timer.run();
  handleKeypad();
}
