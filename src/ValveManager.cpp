#include <ValveManager.h>
#include <MqttManager.h>

ValveManager::ValveManager(PubSubClient *mqttClient)
{
    this->mqttClient = mqttClient;

    // Default closed
    digitalWrite(VALVE_1_PIN, VALVE_CLOSE);
    digitalWrite(VALVE_2_PIN, VALVE_CLOSE);

    pinMode(VALVE_1_PIN, OUTPUT);
    pinMode(VALVE_2_PIN, OUTPUT);

    toggleValve1(false);
    toggleValve2(false);
}

bool ValveManager::isValve1Open()
{
    return valve1Open;
}
void ValveManager::toggleValve1(bool open)
{
    if (open)
    {
        digitalWrite(VALVE_1_PIN, VALVE_OPEN);
        valve1Open = true;
        Serial.println("[ValveManager] Valve 1 opened");
    }
    else
    {
        digitalWrite(VALVE_1_PIN, VALVE_CLOSE);
        valve1Open = false;
        Serial.println("[ValveManager] Valve 1 closed");
    }
    // mqttClient->publish(MQTT_TOPIC_VALVE1_STATE, (isValve1Open() ? VALVE_OPEN_COMMAND : VALVE_CLOSE_COMMAND));
}

bool ValveManager::isValve2Open()
{
    return valve2Open;
}
void ValveManager::toggleValve2(bool open)
{
    if (open)
    {
        digitalWrite(VALVE_2_PIN, VALVE_OPEN);
        valve1Open = true;
        Serial.println("[ValveManager] Valve 2 opened");
    }
    else
    {
        digitalWrite(VALVE_2_PIN, VALVE_CLOSE);
        valve1Open = false;
        Serial.println("[ValveManager] Valve 2 closed");
    }
    // mqttClient->publish(MQTT_TOPIC_VALVE2_STATE, (isValve2Open() ? VALVE_OPEN_COMMAND : VALVE_CLOSE_COMMAND));
}
