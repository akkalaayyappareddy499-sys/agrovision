#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// ================= WIFI =================
const char* WIFI_SSID = "Noor";
const char* WIFI_PASSWORD = "Dora bujii";

// ================= PINS =================
#define DHT_PIN       4
#define DHT_TYPE      DHT11

#define RAIN_PIN      32
#define SOIL_PIN      34
#define LDR_PIN       33

#define BUZZER_PIN    19
#define RELAY_PIN     18
#define FLOW_PIN      27

// ================= SETTINGS =================
#define SOIL_DRY_VALUE  3000
#define SOIL_WET_VALUE   360

#define RAIN_THRESHOLD  1500

// Most relay modules are active LOW
#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

// ================= OBJECTS =================
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

// ================= FLOW SENSOR =================
volatile unsigned long pulseCount = 0;

unsigned long lastFlowTime = 0;

float flowRate = 0.0;
float totalLitres = 0.0;

// ================= CURRENT SENSOR DATA =================
float currentTemperature = 0.0;
float currentHumidity = 0.0;

int currentRainValue = 0;
bool currentRain = false;

int currentSoil = 0;
int currentSoilPercent = 0;
String currentSoilStatus = "UNKNOWN";

bool currentLight = false;
bool currentPump = false;

float currentFlowRate = 0.0;
float currentTotalLitres = 0.0;

// ================= SOIL STATUS =================
String previousSoilStatus = "";

// ================= FLOW INTERRUPT =================
void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

// ================= SOIL READING =================
int readSoilSensor()
{
  long total = 0;

  for (int i = 0; i < 10; i++)
  {
    total += analogRead(SOIL_PIN);
    delay(5);
  }

  return total / 10;
}

// ================= DRY BUZZER =================
// Dry = 3 beeps
void dryBuzzer()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(250);

    digitalWrite(BUZZER_PIN, LOW);
    delay(250);
  }
}

// ================= WET BUZZER =================
// Wet = 2 beeps
void wetBuzzer()
{
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(250);

    digitalWrite(BUZZER_PIN, LOW);
    delay(250);
  }
}

// ================= WIFI =================
void connectWiFi()
{
  Serial.println();
  Serial.println("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 40)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("======================================");
    Serial.println("       WIFI CONNECTED");
    Serial.println("======================================");

    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Open this in browser:");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/api/data");

    Serial.println("======================================");
  }
  else
  {
    Serial.println("WiFi connection FAILED!");
  }
}

// ================= API =================
void sendSensorData()
{
  // CORS
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  String json = "{";

  // Temperature
  json += "\"temperature\":";

  if (isnan(currentTemperature))
  {
    json += "null";
  }
  else
  {
    json += String(currentTemperature, 1);
  }

  json += ",";

  // Humidity
  json += "\"humidity\":";

  if (isnan(currentHumidity))
  {
    json += "null";
  }
  else
  {
    json += String(currentHumidity, 1);
  }

  json += ",";

  // Rain
  json += "\"rainValue\":";
  json += String(currentRainValue);

  json += ",";

  json += "\"rain\":";
  json += (currentRain ? "true" : "false");

  json += ",";

  // Soil
  json += "\"soil\":";
  json += String(currentSoil);

  json += ",";

  json += "\"soilPercent\":";
  json += String(currentSoilPercent);

  json += ",";

  json += "\"soilStatus\":\"";
  json += currentSoilStatus;
  json += "\"";

  json += ",";

  // Light
  json += "\"light\":";
  json += (currentLight ? "true" : "false");

  json += ",";

  // Pump
  json += "\"pump\":";
  json += (currentPump ? "true" : "false");

  json += ",";

  // Flow
  json += "\"flowRate\":";
  json += String(currentFlowRate, 2);

  json += ",";

  json += "\"totalLitres\":";
  json += String(currentTotalLitres, 2);

  json += "}";

  server.send(200, "application/json", json);
}

// ================= ROOT PAGE =================
void handleRoot()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");

  String message = "";

  message += "<html>";
  message += "<head>";
  message += "<title>AgroVision ESP32</title>";
  message += "</head>";

  message += "<body>";

  message += "<h1>AgroVision AI - ESP32</h1>";

  message += "<h2>Smart Irrigation System</h2>";

  message += "<p>ESP32 is connected successfully.</p>";

  message += "<p><a href='/api/data'>Open Sensor API</a></p>";

  message += "</body>";
  message += "</html>";

  server.send(200, "text/html", message);
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("======================================");
  Serial.println("     AGROVISION AI");
  Serial.println(" SMART IRRIGATION MONITORING SYSTEM");
  Serial.println("======================================");

  // DHT
  dht.begin();

  // Pins
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(RELAY_PIN, OUTPUT);

  pinMode(LDR_PIN, INPUT_PULLUP);

  pinMode(FLOW_PIN, INPUT_PULLUP);

  // Initial states
  digitalWrite(BUZZER_PIN, LOW);

  digitalWrite(RELAY_PIN, RELAY_OFF);

  // Flow interrupt
  attachInterrupt(
    digitalPinToInterrupt(FLOW_PIN),
    pulseCounter,
    RISING
  );

  // WiFi
  connectWiFi();

  // ================= WEB SERVER =================

  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/data", HTTP_GET, sendSensorData);

  // OPTIONS request for CORS
  server.on("/api/data", HTTP_OPTIONS, []()
  {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

    server.send(204);
  });

  server.begin();

  Serial.println("Web Server Started!");

  Serial.println();
  Serial.println("API:");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/api/data");

  Serial.println();

  delay(2000);
}

// ================= LOOP =================
void loop()
{
  // Keep web server running
  server.handleClient();

  // ================= READ DHT =================

  float temperature = dht.readTemperature();

  float humidity = dht.readHumidity();

  // ================= READ RAIN =================

  int rainValue = analogRead(RAIN_PIN);

  bool rainDetected = rainValue < RAIN_THRESHOLD;

  // ================= READ SOIL =================

  int soilValue = readSoilSensor();

  int soilPercent = map(
    soilValue,
    SOIL_DRY_VALUE,
    SOIL_WET_VALUE,
    0,
    100
  );

  soilPercent = constrain(
    soilPercent,
    0,
    100
  );

  // ================= SOIL STATUS =================

  String soilStatus;

  if (soilPercent <= 30)
  {
    soilStatus = "DRY";
  }
  else if (soilPercent <= 60)
  {
    soilStatus = "NORMAL";
  }
  else
  {
    soilStatus = "WET";
  }

  // ================= LIGHT =================

  int lightValue = digitalRead(LDR_PIN);

  bool lightBright = (lightValue == LOW);

  // ================= PUMP CONTROL =================

  bool pumpOn = false;

  if (rainDetected)
  {
    // If rain detected → pump OFF
    pumpOn = false;
  }
  else if (soilStatus == "DRY")
  {
    // Dry soil → pump ON
    pumpOn = true;
  }
  else
  {
    // Normal/Wet → pump OFF
    pumpOn = false;
  }

  // Relay
  if (pumpOn)
  {
    digitalWrite(RELAY_PIN, RELAY_ON);
  }
  else
  {
    digitalWrite(RELAY_PIN, RELAY_OFF);
  }

  // ================= BUZZER =================

  // Only beep when soil status changes
  if (soilStatus != previousSoilStatus)
  {
    Serial.println();
    Serial.println("***** SOIL STATUS CHANGED *****");

    if (soilStatus == "DRY")
    {
      Serial.println("DRY -> BUZZER 3 TIMES");

      dryBuzzer();
    }
    else if (soilStatus == "WET")
    {
      Serial.println("WET -> BUZZER 2 TIMES");

      wetBuzzer();
    }

    previousSoilStatus = soilStatus;
  }

  // ================= FLOW SENSOR =================

  unsigned long currentMillis = millis();

  if (currentMillis - lastFlowTime >= 1000)
  {
    lastFlowTime = currentMillis;

    noInterrupts();

    unsigned long pulses = pulseCount;

    pulseCount = 0;

    interrupts();

    // Approximate YF-S201 formula
    flowRate = pulses / 7.5;

    totalLitres += flowRate / 60.0;
  }

  // ================= SAVE CURRENT DATA =================

  if (!isnan(temperature))
  {
    currentTemperature = temperature;
  }

  if (!isnan(humidity))
  {
    currentHumidity = humidity;
  }

  currentRainValue = rainValue;

  currentRain = rainDetected;

  currentSoil = soilValue;

  currentSoilPercent = soilPercent;

  currentSoilStatus = soilStatus;

  currentLight = lightBright;

  currentPump = pumpOn;

  currentFlowRate = flowRate;

  currentTotalLitres = totalLitres;

  // ================= SERIAL MONITOR =================

  Serial.println();
  Serial.println("----------- SENSOR DATA -----------");

  Serial.print("WiFi        : ");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("CONNECTED");
  }
  else
  {
    Serial.println("DISCONNECTED");
  }

  Serial.print("IP Address  : ");
  Serial.println(WiFi.localIP());

  Serial.print("Temperature : ");

  if (isnan(temperature))
  {
    Serial.println("ERROR");
  }
  else
  {
    Serial.print(temperature);
    Serial.println(" C");
  }

  Serial.print("Humidity    : ");

  if (isnan(humidity))
  {
    Serial.println("ERROR");
  }
  else
  {
    Serial.print(humidity);
    Serial.println(" %");
  }

  Serial.print("Rain Value  : ");
  Serial.println(rainValue);

  Serial.print("Rain        : ");

  if (rainDetected)
  {
    Serial.println("DETECTED");
  }
  else
  {
    Serial.println("NO RAIN");
  }

  Serial.print("Soil Raw    : ");
  Serial.println(soilValue);

  Serial.print("Soil        : ");
  Serial.print(soilPercent);
  Serial.println(" %");

  Serial.print("Soil Level  : ");
  Serial.println(soilStatus);

  Serial.print("Light       : ");

  if (lightBright)
  {
    Serial.println("BRIGHT");
  }
  else
  {
    Serial.println("DARK");
  }

  Serial.print("Pump        : ");

  if (pumpOn)
  {
    Serial.println("ON");
  }
  else
  {
    Serial.println("OFF");
  }

  Serial.print("Flow Rate   : ");
  Serial.print(flowRate);
  Serial.println(" L/min");

  Serial.print("Water Used  : ");
  Serial.print(totalLitres, 2);
  Serial.println(" L");

  Serial.println("-----------------------------------");

  // Small delay
  delay(1000);
}