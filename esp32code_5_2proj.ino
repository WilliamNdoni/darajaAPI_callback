#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "Galaxy S21 FE";
const char* password = "william77";

// MQTT Broker settings
const char* mqtt_server = "4761eba0b4eb4a958f76528215b690db.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "williamndoni"; 
const char* mqtt_password = "William77"; 
const char* mqtt_client_id = "ESP32_MPESA_CLIENT";
const char* mqtt_topic = "wuzu58mpesa_data";

// LCD Settings
#define LCD_ADDRESS 0x27
#define LCD_COLS 20
#define LCD_ROWS 4
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Queue definitions
#define MAX_QUEUE_SIZE 5
String messageQueue[MAX_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
bool pumpRunning = false; // Flag to check if the pump is running

// Non-blocking pump control variables
unsigned long pumpStartTime = 0;
unsigned long pumpDuration = 0;
bool pumpActive = false;

// Non-blocking countdown variables
unsigned long countdownStartTime = 0;
int countdownSeconds = 10;
bool countdownActive = false;
int currentCountdown = 0;
bool displayedCountdown = false;

// Current transaction info
String currentFirstName = "";
float currentAmount = 0;

// Relay Pin
const int relayPin = 5;

// Reset timer (every 6 hours)
unsigned long lastResetTime = 0;
const unsigned long resetInterval = 6 * 60 * 60 * 1000;

void setup_wifi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected!");
  lcd.setCursor(0, 1);
  lcd.print("IP: ");
  lcd.print(WiFi.localIP());
  
  Serial.println("\nWiFi connected");
}

void displayTransactionInfo(const char* firstName, float amount) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Customer:");
  lcd.setCursor(0, 1);
  lcd.print(firstName);
  
  lcd.setCursor(0, 2);
  lcd.print("Amount Paid:");
  lcd.setCursor(0, 3);
  lcd.print("KES ");
  lcd.print(amount);
}

void startCountdown() {
  lcd.clear();
  
  // Display customer name in the instruction
  lcd.setCursor(0, 0);
  lcd.print(currentFirstName);
  lcd.print(", PLACE");
  
  lcd.setCursor(0, 1);
  lcd.print("CONTAINER UNDER");
  
  lcd.setCursor(0, 2);
  lcd.print("THE NOZZLE");
  
  countdownStartTime = millis();
  countdownActive = true;
  currentCountdown = countdownSeconds;
  displayedCountdown = false;
}

void updateCountdown() {
  if (!countdownActive) return;
  
  unsigned long elapsed = millis() - countdownStartTime;
  int newCountdown = countdownSeconds - (elapsed / 1000);
  
  if (newCountdown != currentCountdown || !displayedCountdown) {
    currentCountdown = newCountdown;
    displayedCountdown = true;
    
    lcd.setCursor(9, 3);
    lcd.print("    ");
    lcd.setCursor(9, 3);
    if (currentCountdown < 10) lcd.print(" ");
    lcd.print(currentCountdown);
    lcd.print("...");
  }
  
  if (currentCountdown <= 0) {
    countdownActive = false;
    lcd.setCursor(0, 3);
    lcd.print("STARTING PUMP...");
    
    // Start the pump after countdown
    pumpStartTime = millis();
    pumpDuration = calculatePumpTime(currentAmount);
    pumpActive = true;
    digitalWrite(relayPin, HIGH);
    Serial.println("Starting pump for " + String(pumpDuration) + "ms");
  }
}

int calculatePumpTime(float amount) {
  float litres = amount / 60;
  return (litres / 0.02) * 1000;
}

void showWelcomeMessage() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Welcome!");
  lcd.setCursor(1, 1);
  lcd.print("Pay to get your milk");
}

void printQueue() {
  Serial.println("Current Queue:");
  for (int i = queueHead; i != queueTail; i = (i + 1) % MAX_QUEUE_SIZE) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(messageQueue[i]);
  }
  Serial.println("-------------------------");
}

void validateAndEnqueueMessage(String message) {
  DynamicJsonDocument testDoc(1024);
  DeserializationError error = deserializeJson(testDoc, message);
  if (error) {
    Serial.print("Invalid JSON message received: ");
    Serial.println(error.c_str());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Invalid message");
    lcd.setCursor(0, 1);
    lcd.print("Rejected");
    delay(2000);
    return;
  }
  
  if (!testDoc.containsKey("first_name") || !testDoc.containsKey("amount") || !testDoc.containsKey("pump_time_ms")) {
    Serial.println("Message missing required fields");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Invalid data");
    lcd.setCursor(0, 1);
    lcd.print("Missing fields");
    delay(2000);
    return;
  }

  enqueueMessage(message);
}

void enqueueMessage(String message) {
  if ((queueTail + 1) % MAX_QUEUE_SIZE == queueHead) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Queue full!");
    lcd.setCursor(0, 1);
    lcd.print("Message dropped");
    delay(2000);
    Serial.println("Queue is full. Message dropped.");
    return;
  }

  messageQueue[queueTail] = message;
  queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
  saveQueueToSPIFFS();

  Serial.println("Message added to queue.");
  printQueue();

  if (!pumpRunning && !pumpActive && !countdownActive) {
    Serial.println("Starting to process new transaction");
    processNextTransaction();
  } else {
    Serial.println("System busy, transaction queued for later processing");
  }
}

void processNextTransaction() {
  if (queueHead == queueTail) {
    pumpRunning = false;
    Serial.println("Queue empty, no transactions to process");
    showWelcomeMessage();
    return;
  }
  
  pumpRunning = true;
  String message = messageQueue[queueHead];
  startTransaction(message);
}

String dequeueMessage() {
  if (queueHead == queueTail) {
    Serial.println("Queue is empty.");
    return "";
  }

  String message = messageQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
  saveQueueToSPIFFS();

  Serial.println("Dequeued Message:");
  Serial.println(message);
  printQueue();

  return message;
}

void callback(char* topic, byte* payload, unsigned int length) {
  lcd.backlight();
  Serial.print("MQTT Message Arrived [");
  Serial.print(topic);
  Serial.print("]: ");

  String receivedMessage = "";
  for (int i = 0; i < length; i++) {
    receivedMessage += (char)payload[i];
  }
  Serial.println(receivedMessage);

  validateAndEnqueueMessage(receivedMessage);
}

void startTransaction(String message) {
  Serial.println("Starting Transaction:");
  Serial.println(message);

  if (message.length() < 5) {
    Serial.println("Message too short or empty, skipping");
    dequeueMessage();
    pumpRunning = false;
    processNextTransaction();
    return;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("JSON Error:");
    lcd.setCursor(0, 1);
    lcd.print(error.c_str());
    delay(2000);
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    dequeueMessage();
    pumpRunning = false;
    processNextTransaction();
    return;
  }

  currentFirstName = doc["first_name"].as<String>();
  currentAmount = doc["amount"];

  displayTransactionInfo(currentFirstName.c_str(), currentAmount);

  if (currentAmount >= 10) {
    startCountdown();
  } else {
    Serial.println("Insufficient funds. No pumping.");
    dequeueMessage();
    pumpRunning = false;
    processNextTransaction();
  }
}

void finishTransaction() {
  Serial.println("Transaction complete.");
  dequeueMessage();
  pumpRunning = false;
    
  // Check for more transactions
  if (queueHead != queueTail) {
    Serial.println("Processing next transaction in queue");
    processNextTransaction();
  } else {
    Serial.println("Queue empty, showing welcome message");
    showWelcomeMessage();
  }
}

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

void saveQueueToSPIFFS() {
  File file = SPIFFS.open("/queue.txt", "w");
  if (!file) {
    Serial.println("Failed to save queue!");
    return;
  }

  for (int i = queueHead; i != queueTail; i = (i + 1) % MAX_QUEUE_SIZE) {
    file.println(messageQueue[i]);
  }

  file.close();
}

void loadQueueFromSPIFFS() {
  File file = SPIFFS.open("/queue.txt", "r");
  if (!file) {
    Serial.println("Failed to open queue for reading!");
    return;
  }

  while (file.available()) {
    String message = file.readStringUntil('\n');
    validateAndEnqueueMessage(message);
  }

  file.close();
}

void setup() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SPIFFS Error!");
    delay(2000);
    return;
  }

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  lcd.init();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  Serial.begin(115200);
  setup_wifi();
  
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  loadQueueFromSPIFFS();

  if (queueHead == queueTail) {
    showWelcomeMessage();
  }
}

void loop() {
  // Handle MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Update countdown if active
  if (countdownActive) {
    updateCountdown();
  }

  // Check pump status
  if (pumpActive && (millis() - pumpStartTime >= pumpDuration)) {
    digitalWrite(relayPin, LOW);
    pumpActive = false;
    Serial.println("Milk pumping completed.");
    finishTransaction();
  }

  // Periodic queue reset
  unsigned long currentMillis = millis();
  if (currentMillis - lastResetTime >= resetInterval) {
    Serial.println("Resetting Queue...");
    lastResetTime = currentMillis;
    queueHead = queueTail = 0;
    saveQueueToSPIFFS();
    showWelcomeMessage();
  }
}