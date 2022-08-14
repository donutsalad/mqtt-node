#include "commons.h"

#define MQTT_INCOMING_REQUEST_QUEUE_LENGTH  10
#define MQTT_OUTGOING_DATA_QUEUE_LENGTH     10

typedef struct MQTTIncomingData_Parsed {
    hash_t client_id;
    hash_t route;
    hash_t path;
    char stem[16];

    hash_t data_tag;
    char data[256]; //Need to bring these magic numbers out
} mqtt_request_t;

typedef struct MQTTOutgoingData {
    char* app_tag;
    char* data_tag;

    char stem[16];
    char data[256];
} mqtt_outgoing_t;


mqtt_request_t *GetIncomingBufferBlock();
void FreeIncomingBufferBlock(mqtt_request_t *buffer);
void ZeroIncomingBufferBlock(mqtt_request_t *buffer);
mqtt_outgoing_t *GetOutgoingBufferBlock();
void FreeOutgoingBufferBlock(mqtt_outgoing_t *buffer);
void ZeroOutgoingBufferBlock(mqtt_outgoing_t *buffer);
void CleanupIncomingBuffer();
void CleanOneIncomingBuffer();
void CleanupOutgoingBuffer();
void CleanOneOutgoingBuffer();
void CleanAllBuffers();