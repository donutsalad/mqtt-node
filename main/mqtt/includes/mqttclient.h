#include "commons.h"

#include <freertos/event_groups.h>

#define MQTT_NODE_KEEPALIVE     120
#define MQTT_NODE_TASK_PRIORITY 5
#define MQTT_NODE_TASK_STACK    6144
#define MQTT_NODE_PORT          1883
#define MQTT_UNDEFINED_TAG      "undefined"

#define MQTT_MANUAL_RECONNECT   true
#define MQTT_MAX_RECON_ATTEMPTS 4

#define MQTT_START_STACK_SIZE   4096

#define MQTT_START_SUCCESS      0
#define MQTT_START_DUPLICATE    1
#define MQTT_START_FAILURE      2

#define MQTT_EVENT_SERVICE_RDY  BIT0
#define MQTT_EVENT_PCTASK_CALL  BIT1
#define MQTT_EVENT_PCTASK_DONE  BIT2
#define MQTT_EVENT_TERMINATING  BIT3

EventGroupHandle_t get_mqtt_event_group(void);

int StartMQTTClient(
    void (*valid_connection)(void),
    void (*unsuccessful)(void),
    void (*ungraceful_shutdown)(void)
);