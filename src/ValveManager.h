#include <PubSubClient.h>

class ValveManager
{
private:
    PubSubClient *mqttClient;
    bool valve1Open = false;
    bool valve2Open = false;

public:
    const int VALVE_1_PIN = 21;
    const int VALVE_2_PIN = 19;
    const int VALVE_CLOSE = HIGH;
    const int VALVE_OPEN = LOW;
    const char *VALVE_CLOSE_COMMAND = "close";
    const char *VALVE_OPEN_COMMAND = "open";

    ValveManager(PubSubClient *mqttClient);

    void toggleValve1(bool open);
    bool isValve1Open();

    void toggleValve2(bool open);
    bool isValve2Open();
};