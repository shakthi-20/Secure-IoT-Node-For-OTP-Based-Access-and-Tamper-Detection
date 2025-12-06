#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Shakthi";
const char* password = "shakthi13";
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);

#define PIR_PIN 13

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("LockerEndNode")) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  int pirState = digitalRead(PIR_PIN);
  if (pirState == HIGH) {
    Serial.println("🚶 Motion detected! Publishing MQTT message...");
    client.publish("parcelLocker/pir", "motion_detected");
    delay(5000); // wait to avoid flooding MQTT
  }
}
