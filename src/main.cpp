#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <AsyncMessagePack.h>
#include <ElegantOTA.h>
#include <ValveManager.h>

#define USE_SERIAL Serial

const char *WIFISSID = "Die Burg";                 // WiFi SSID name
const char *WIFIPASS = "InternetFuerFamilieKorte"; // WiFi Password
const char *HOSTNAME = "valvecontroller";          // Hostname

const char *MQTT_BROKER = "192.168.178.29"; // IP of the MQTT Broker
const char *MQTT_TOPIC_VALVE1_STATE = "valvecontroller/valve1/state";
const char *MQTT_TOPIC_VALVE2_STATE = "valvecontroller/valve2/state";
const char *MQTT_TOPIC_VALVE1_COMMAND = "valvecontroller/valve1/command";
const char *MQTT_TOPIC_VALVE2_COMMAND = "valvecontroller/valve2/command";
const char *MQTT_TOPIC_FLOW_RATE = "valvecontroller/flow_rate";
const char *MQTT_TOPIC_IS_FLOWING = "valvecontroller/is_flowing";
const char *MQTT_TOPIC_ERROR = "valvecontroller/error";
const char *MQTT_CLIENT_NAME = "valvecontroller";

const long FLOW_SENSOR_MEASUREMENT_INTERVAL = 1000;
const long FLOW_SENSOR_PUBLISH_INTERVAL = 1000 * 10;
long flowSensorLastPublish = 0;
long flowSensorLastReading = 0;
bool isFlowing = false;
volatile int flowRateSensorInterruptCount = 0; // Measuring the rising edges of the signal
float flowRatePerMinute = 0;
float flowRateUnitPerPulse = 1;
float flowRatePerMinuteLastPublished = 0;

const int VALVE_CLOSE = HIGH;
const int VALVE_OPEN = LOW;

const bool wifiEnabled = true;

AsyncWebServer webServer(80); // AsyncWebServer  on port 80

const long GLOBAL_DELAY = 150;

const int VALVE_1_PIN = 21;
const int VALVE_2_PIN = 19;
const int FLOW_RATE_SENSOR_PIN = 23;

const char *VALVE_OPEN_COMMAND = "open";
const char *VALVE_CLOSE_COMMAND = "close";

bool valve1Open = false;
bool valve2Open = false;

bool isFirstRun = true;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// MqttManager mqttManager(espClient);
// ValveManager valveManager(&mqttClient);

void setupWifi()
{
  delay(100);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(WIFISSID);
  Serial.print(" as ");
  Serial.print(HOSTNAME);
  Serial.println();
  delay(100);

  // Set new hostname
  WiFi.hostname(HOSTNAME);

  WiFi.begin(WIFISSID, WIFIPASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(5000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
  Serial.println(HOSTNAME);
}

void toggleValve1(bool open)
{
  if (open)
  {
    digitalWrite(VALVE_1_PIN, VALVE_OPEN);
    valve1Open = true;
    Serial.println("Valve 1 opened");
  }
  else
  {
    digitalWrite(VALVE_1_PIN, VALVE_CLOSE);
    valve1Open = false;
    Serial.println("Valve 1 closed");
  }
  mqttClient.publish(MQTT_TOPIC_VALVE1_STATE, valve1Open ? "open" : "closed");
}

void toggleValve2(bool open)
{
  if (open)
  {
    digitalWrite(VALVE_2_PIN, VALVE_OPEN);
    valve2Open = true;
    Serial.println("Valve 2 opened");
  }
  else
  {
    digitalWrite(VALVE_2_PIN, VALVE_CLOSE);
    valve2Open = false;
    Serial.println("Valve 2 closed");
  }
  mqttClient.publish(MQTT_TOPIC_VALVE2_STATE, valve1Open ? "open" : "closed");
}

void mqttCallback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Check incomming message for interesting topics
  if (String(topic) == MQTT_TOPIC_VALVE1_COMMAND)
  {
    toggleValve1(messageTemp == VALVE_OPEN_COMMAND);
  }
  else if (String(topic) == MQTT_TOPIC_VALVE2_COMMAND)
  {
    toggleValve2(messageTemp == VALVE_OPEN_COMMAND);
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    if (mqttClient.connect(MQTT_CLIENT_NAME))
    {
      // if (mqttClient.connect(MQTT_CLIENT_NAME, mqtt_user, mqtt_password)) {
      Serial.println("MQTT connected");

      Serial.println(MQTT_TOPIC_VALVE1_COMMAND);
      mqttClient.subscribe(MQTT_TOPIC_VALVE1_COMMAND, 1); // QoS = 1 (at least once)
      Serial.print("Subscribing to MQTT topic ");
      Serial.println(MQTT_TOPIC_VALVE2_COMMAND);
      mqttClient.subscribe(MQTT_TOPIC_VALVE2_COMMAND, 1); // QoS = 1 (at least once)
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupApiServer()
{

  // // Route for root / web page
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    Serial.println("HTTP GET request for / ...");
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    root["valve1"] = valve1Open ? "open" : "closed";
    root["valve2"] = valve2Open ? "open" : "closed";
    root["flow_rate"] = flowRatePerMinute;
    root["flow_rate_unit"] = "l/min";
    root["is_flowing"] = isFlowing;
    serializeJson(root, *response);
    request->send(response); });

  webServer.on("/set", HTTP_GET, [](AsyncWebServerRequest *request)
               {


    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    JsonObject root = doc.to<JsonObject>();
    
    Serial.println("HTTP GET request for /set ...");
    const AsyncWebParameter* valveIdParam = request->getParam("valveId");
    const AsyncWebParameter* commandParam = request->getParam("command");

    String valveId = valveIdParam != NULL ? valveIdParam->value() : "";
    String command = commandParam != NULL ? commandParam->value() : "";


    bool valveIdValid = false;
    bool commandValid = false;
    bool commandToSend = false;

    if (valveId == "") {
      root["status"] = "error";
      root["error"] = "No valveId provided";
      response->setCode(400);
    } else if (command == "") {
      root["status"] = "error";
      root["error"] = "No command provided";
      response->setCode(400);
    } else {
      commandValid = command == "open" || command == "close";
      commandToSend = command == "open";
    }

    if (commandValid) {
      if (valveId == "1")
      {
        valveIdValid = true;
        toggleValve1(commandToSend);
      }
      if (valveId == "2")
      {
        valveIdValid = true;
        toggleValve2(commandToSend);
      }
    }
    if (valveIdValid && commandValid)
    {
      root["status"] = "success";
    } else if (!commandValid) {
      root["status"] = "error";
      root["error"] = "Invalid command '" + command + "'";
      response->setCode(400);
    } else if (!valveIdValid) { 
      root["status"] = "error";
      root["error"] = "Invalid valve id '" + valveId + "'";
      response->setCode(400);
    } else {
      root["status"] = "error";
      root["error"] = "Unknown error";
      response->setCode(500);
    }
    serializeJson(root, *response);
    request->send(response); });
}

void flowSensorInterrupt() // This is the function that the interupt calls
{
  flowRateSensorInterruptCount++; // This function measures the rising and falling edge of the hall effect sensors signal
}

void setupFlowRateSensor()
{
  // Sensor is PNP -> Pull down and rising
  pinMode(FLOW_RATE_SENSOR_PIN, INPUT_PULLDOWN);                      // initializes digital pin 2 as an input
  attachInterrupt(FLOW_RATE_SENSOR_PIN, flowSensorInterrupt, RISING); // and the interrupt is attached
}

void readFlowRateSensor()
{
  if (millis() - flowSensorLastReading < FLOW_SENSOR_MEASUREMENT_INTERVAL)
  {
    return;
  }
  flowSensorLastReading = millis();
  // Serial.println("flowRateSensorInterruptCount: " + (String)flowRateSensorInterruptCount);

  noInterrupts();
  float flowRatePerHour = (flowRateSensorInterruptCount * 60) / flowRateUnitPerPulse; //(Pulse frequency x 60) / Q, = flow rate in L/hour
  flowRatePerMinute = flowRatePerHour / 60;
  flowRateSensorInterruptCount = 0;
  interrupts();

  if (flowRatePerMinute > 0)
  {
    isFlowing = true;
  }
  else
  {
    isFlowing = false;
  }
  if (millis() - flowSensorLastPublish > FLOW_SENSOR_PUBLISH_INTERVAL)
  {
    if (abs(flowRatePerMinute - flowRatePerMinuteLastPublished) > 0.1)
    {
      Serial.println("Current flow rate: " + (String)flowRatePerMinute + " l/min, is flowing: " + (isFlowing ? "true" : "false"));
      char result[8];
      dtostrf(flowRatePerMinute, 2, 2, result);

      mqttClient.publish(MQTT_TOPIC_FLOW_RATE, result);
      mqttClient.publish(MQTT_TOPIC_IS_FLOWING, isFlowing ? "true" : "false");
      Serial.println("Publishing: " + String(result) + " l/min, is flowing: " + (isFlowing ? "true" : "false"));
      flowRatePerMinuteLastPublished = flowRatePerMinute;
    }
    flowSensorLastPublish = millis();
  }
}

void setup()
{
  Serial.begin(115200);
  if (wifiEnabled)
  {
    setupWifi();
    Serial.println("Connecting to MQTT");
    mqttClient.setServer(MQTT_BROKER, 1883);
    mqttClient.setCallback(mqttCallback);

    Serial.println("Setting up flor rate sensor");
    setupFlowRateSensor();

    Serial.println("Starting OTA webserver");
    // Enable Over-the-air updates at http://<IPAddress>/update
    ElegantOTA.begin(&webServer);

    Serial.println("Setting up API server");
    setupApiServer();

    Serial.println("Starting webserver");
    // Start server
    webServer.begin();

    delay(100);

    // Default closed
    digitalWrite(VALVE_1_PIN, VALVE_CLOSE);
    pinMode(VALVE_1_PIN, OUTPUT);

    digitalWrite(VALVE_2_PIN, VALVE_CLOSE);
    pinMode(VALVE_2_PIN, OUTPUT);
  }
}

void loop()
{
  if (wifiEnabled)
  {

    // connect to MQTT broker
    if (!mqttClient.connected())
    {
      reconnect();
    }

    if (isFirstRun)
    {
      mqttClient.publish(MQTT_TOPIC_ERROR, "No error");

      toggleValve1(false);
      toggleValve2(false);

      mqttClient.publish(MQTT_TOPIC_FLOW_RATE, "0.00");
      mqttClient.publish(MQTT_TOPIC_IS_FLOWING, "false");

      isFirstRun = false;
    }

    readFlowRateSensor();
    mqttClient.loop();
    ElegantOTA.loop();
  }

  delay(GLOBAL_DELAY);
}
