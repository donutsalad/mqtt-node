#include "commons.h"

#define MQTT_INCOMING_REQUEST_QUEUE_LENGTH  10
#define MQTT_OUTGOING_DATA_QUEUE_LENGTH     10

#define MQTT_INBOX_QUEUE_OKAY   0
#define MQTT_INBOX_QUEUE_FULL   1
#define MQTT_INBOX_QUEUE_FAIL   2

#define MQTT_OUTBOX_QUEUE_OKAY   0
#define MQTT_OUTBOX_QUEUE_FULL   1
#define MQTT_OUTBOX_QUEUE_FAIL   2

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

int AddToInboxQueue_WithTimeout(mqtt_request_t *item, TickType_t timeout);
mqtt_request_t* RecieveFromInboxQueue_WithTimeout(TickType_t timeout);

int AddToInboxQueue(mqtt_request_t *item);
int TryAddToInboxQueue(mqtt_request_t *item);

mqtt_request_t* RecieveFromInboxQueue(void);
mqtt_request_t* TryRecieveFromInboxQueue(void);

int AddToOutboxQueue_WithTimeout(mqtt_outgoing_t *item, TickType_t timeout);
mqtt_outgoing_t* RecieveFromOutboxQueue_WithTimeout(TickType_t timeout);

int AddToOutboxQueue(mqtt_outgoing_t *item);
int TryAddToOutboxQueue(mqtt_outgoing_t *item);

mqtt_outgoing_t* RecieveFromOutboxQueue(void);
mqtt_outgoing_t* TryRecieveFromOutboxQueue(void);

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