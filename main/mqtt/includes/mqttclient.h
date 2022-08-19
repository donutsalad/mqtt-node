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
#define MQTT_STOP_STACK_SIZE    2048
#define MQTT_INVALID_STACK_SIZE 2048
#define MQTT_PC_TASK_STACK_SIZE 4096
#define MQTT_INBOX_STACK_SIZE   4096
#define MQTT_OUTBOX_STACK_SIZE  4096

#define MQTT_START_SUCCESS      0
#define MQTT_START_DUPLICATE    1
#define MQTT_START_FAILURE      2

#define MQTT_INBOX_ALREADY_INIT -1
#define MQTT_INBOX_TASK_CREATED 0
#define MQTT_INBOX_MEMORY_ERROR 1
#define MQTT_INBOX_UNKNOWN_FAIL 2

#define MQTT_OUTBOX_ALREADY_INIT    -1
#define MQTT_OUTBOX_TASK_CREATED    0
#define MQTT_OUTBOX_MEMORY_ERROR    1
#define MQTT_OUTBOX_UNKNOWN_FAIL    2

#define MQTT_EVENT_SERVICE_RDY  BIT0    //MQTT Server Connection Established
#define MQTT_EVENT_PCTASK_CALL  BIT1    //Started initialising appstack
#define MQTT_EVENT_PCTASK_DONE  BIT2    //Finished initialising appstack
#define MQTT_EVENT_TERMINATING  BIT3    //MQTT Services Shutdown in progress

EventGroupHandle_t  get_mqtt_event_group(void);
hash_t              get_mqtt_client_id(void);

int StartMQTTClient(
    void (*valid_connection)(void),
    void (*unsuccessful)(void),
    void (*ungraceful_shutdown)(void)
);

void ShutdownMQTTServices(void);