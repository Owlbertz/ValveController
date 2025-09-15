
#include <PubSubClient.h>
#include <MqttManager.h>

MqttManager::MqttManager(Client &client)
{
    this->mqttClient = PubSubClient(client);
    Serial.println("[MQTT] Setting broker to '" + String(MQTT_BROKER) + "'");
    mqttClient.setServer(MQTT_BROKER, 1883);
}

void MqttManager::publish(const char *topic, const char *value)
{
    Serial.println("[MQTT] Publishing '" + String(value) + "' to '" + String(topic) + "'");
    mqttClient.publish(topic, value);
}

void MqttManager::handleMqttMessage(char *topic, unsigned char *message, unsigned int length)
{
    Serial.print("[MQTT] Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    if (callback)
    {
        callback(topic, messageTemp);
    }
}

void MqttManager::connect()
{
    Serial.println("[MQTT] Attempting initial connection to '" + String(MQTT_BROKER) + "' as '" + String(MQTT_CLIENT_NAME) + "'...");

    Serial.println("[MQTT] Registering callback");
    mqttClient.setCallback(std::bind(&MqttManager::handleMqttMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // mqttClient.setCallback(handleMqttMessage);

    if (mqttClient.connect(MQTT_CLIENT_NAME))
    {
        Serial.println("[MQTT] Connected");
        Serial.print("[MQTT] Subscribing to topic: ");
        Serial.println(MQTT_TOPIC_VALVE1_COMMAND);
        mqttClient.subscribe(MQTT_TOPIC_VALVE1_COMMAND, 1); // QoS = 1 (at least once)
        Serial.print("[MQTT] Subscribing to topic: ");
        Serial.println(MQTT_TOPIC_VALVE2_COMMAND);
        mqttClient.subscribe(MQTT_TOPIC_VALVE2_COMMAND, 1); // QoS = 1 (at least once)
    }
    else
    {
        Serial.print("[MQTT] failed, rc=");
        Serial.println(mqttClient.state());
    }

    Serial.println("[MQTT] Connection done");
}

void MqttManager::loop()
{
    Serial.println("[MQTT] Loop start");
    Serial.print("[MQTT] MQTT state: ");
    Serial.println(mqttClient.state());
    // if (!mqttClient.connected())
    // {
    //     reconnect();
    // }
    mqttClient.loop();
    Serial.println("[MQTT] Loop end");
}

void MqttManager::reconnect()
{
    Serial.print("[MQTT] MQTT state: ");
    Serial.println(mqttClient.state());
    // Loop until we're reconnected
    while (!mqttClient.connected())
    {
        Serial.println("[MQTT] Attempting re-connection to '" + String(MQTT_BROKER) + "' as '" + String(MQTT_CLIENT_NAME) + "'...");
        // Attempt to connect
        if (mqttClient.connect(MQTT_CLIENT_NAME))
        {
            Serial.println("[MQTT] Connected");
            Serial.print("[MQTT] Subscribing to topic: ");
            Serial.println(MQTT_TOPIC_VALVE1_COMMAND);
            mqttClient.subscribe(MQTT_TOPIC_VALVE1_COMMAND, 1); // QoS = 1 (at least once)
            Serial.print("[MQTT] Subscribing to topic: ");
            Serial.println(MQTT_TOPIC_VALVE2_COMMAND);
            mqttClient.subscribe(MQTT_TOPIC_VALVE2_COMMAND, 1); // QoS = 1 (at least once)
        }
        else
        {
            Serial.print("[MQTT] failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println("try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }

    Serial.println("[MQTT] Reconnection completed");
}
