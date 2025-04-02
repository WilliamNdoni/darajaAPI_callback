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

// Queue definitions
#define MAX_QUEUE_SIZE 5
String messageQueue[MAX_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
bool pumpRunning = false;  // Track if the pump is running

// Relay Pin (Adjust the GPIO pin if needed)
const int relayPin = 5;  // Change this pin to the one controlling your relay

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

  // Add message to the queue
  enqueueMessage(String(message));
}

void processTransaction(String message) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract transaction details
  const char* firstName = doc["first_name"];
  const char* middleName = doc["middle_name"];
  const char* lastName = doc["last_name"];
  float amount = doc["amount"];
  int pumpTimeMs = doc["pump_time_ms"];

  Serial.println("Processing Transaction...");
  Serial.print("First Name: ");
  Serial.println(firstName);
  Serial.print("Middle Name: ");
  Serial.println(middleName);
  Serial.print("Last Name: ");
  Serial.println(lastName);
  Serial.print("Amount: ");
  Serial.println(amount);
  Serial.print("Pump time: ");
  Serial.println(pumpTimeMs);

  // Start pump if sufficient amount
  if (amount >= 10) {
    Serial.println("Starting pump...");
    digitalWrite(relayPin, HIGH);  // Turn on the relay (pump)
    delay(pumpTimeMs);             // Run pump for the time specified in the message
    digitalWrite(relayPin, LOW);   // Turn off the relay (pump)
    Serial.println("Transaction completed - Milk pumped");
  } else {
    Serial.println("Insufficient payment. Skipping...");
  }

  // After pump finishes, process next message if any
  pumpRunning = false;
  if (queueHead != queueTail) {
    // Dequeue the next message and process it
    String nextMessage = dequeueMessage();
    pumpRunning = true;
    processTransaction(nextMessage);
  }
}

String dequeueMessage() {
  if (queueHead == queueTail) {
    Serial.println("Queue is empty.");
    return "";  // No messages in the queue
  }

  String message = messageQueue[queueHead];

  // Clear the processed message from the queue
  messageQueue[queueHead] = "";  // Delete the message by replacing with empty string

  // Move the head to the next position
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;

  return message;
}

void enqueueMessage(String message) {
  if ((queueTail + 1) % MAX_QUEUE_SIZE == queueHead) {
    Serial.println("Queue is full. Message dropped.");
    return;  // Queue is full, drop the message
  }
  messageQueue[queueTail] = message;
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  Serial.println("Message added to the queue.");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic, 1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(relayPin, OUTPUT);  // Set the relay pin as output
  digitalWrite(relayPin, LOW); // Ensure the relay is off initially
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  if (!pumpRunning && queueHead != queueTail) {
    // If pump is not running, process the next message in the queue
    String nextMessage = dequeueMessage();
    pumpRunning = true;
    processTransaction(nextMessage);
  }
}
