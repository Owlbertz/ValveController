#include <PubSubClient.h>

class MqttManager
{
private:
    PubSubClient mqttClient;
    void reconnect();
    void handleMqttMessage(char *topic, unsigned char *message, unsigned int length);
    std::function<void(char *, String)> callback;

public:
    const char *MQTT_BROKER = "192.168.178.29"; // IP of the MQTT Broker
    const char *MQTT_TOPIC_VALVE1_STATE = "valvecontroller/valve1/state";
    const char *MQTT_TOPIC_VALVE2_STATE = "valvecontroller/valve2/state";
    const char *MQTT_TOPIC_VALVE1_COMMAND = "valvecontroller/valve1/command";
    const char *MQTT_TOPIC_VALVE2_COMMAND = "valvecontroller/valve2/command";
    const char *MQTT_TOPIC_FLOW_RATE = "valvecontroller/flow_rate";
    const char *MQTT_TOPIC_ERROR = "valvecontroller/error";
    const char *MQTT_CLIENT_NAME = "valvecontroller";

    MqttManager(Client &client);
    void publish(const char *topic, const char *value);
    void connect();
    void addCallback(std::function<void(char *, String)>);
    void loop();
};