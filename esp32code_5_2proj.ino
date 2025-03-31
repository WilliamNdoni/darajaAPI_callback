#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// WiFi credentials
const char* ssid = "Galaxy S21 FE";
const char* password = "william77";
  
// MQTT Broker settings
const char* mqtt_server = "4761eba0b4eb4a958f76528215b690db.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "williamndoni"; // If required
const char* mqtt_password = "William77"; // If required
const char* mqtt_client_id = "ESP32_MPESA_CLIENT";
const char* mqtt_topic = "wuzu58mpesa_data";

WiFiClientSecure espClient;
PubSubClient client(espClient);

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

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  // Create buffer for the message
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  
  Serial.println(message);
  
  // Parse JSON message
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract transaction details
  const char* transactionName = doc["first_name"];
  float amount = doc["amount"];
  // const char* phoneNumber = doc["phoneNumber"];
  // const char* timestamp = doc["timestamp"];
  
  Serial.println("Transaction details:");
  Serial.print("First Name: ");
  Serial.println(transactionName);
  Serial.print("Amount: ");
  Serial.println(amount);
  // Serial.print("Phone: ");
  // Serial.println(phoneNumber);
  // Serial.print("Time: ");
  // Serial.println(timestamp);
  
  // Here you can add code to process the transaction
  // For example, control relays, update displays, etc.
  // processTransaction(transactionId, amount, phoneNumber);(for use later)
}

// void processTransaction(const char* id, float amount, const char* phone) {
//   // Add your transaction processing logic here
//   // For example:
//   if (amount >= 100) {
//     // Turn on a relay
//     digitalWrite(5, HIGH);
//     Serial.println("Access granted - payment received");
//     delay(5000); // Keep on for 5 seconds
//     digitalWrite(5, LOW);
//   }
// }

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // pinMode(5, OUTPUT); // Example pin for controlling something
  Serial.begin(115200);
  setup_wifi();
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}