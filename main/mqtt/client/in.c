#include "mqttclient.h"
#include "mqttinterface.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

static TaskHandle_t xMQTTInboxHandle = NULL;
static QueueHandle_t xMQTTInboxQueue = NULL;

static inline int _queue_result(mqtt_request_t* item, TickType_t timeout) {
    switch(xQueueSendToBack(xMQTTInboxQueue, (void*) item, timeout)) {
        case pdPASS:        return MQTT_INBOX_QUEUE_OKAY;
        case errQUEUE_FULL: return MQTT_INBOX_QUEUE_FULL;
        default:            return MQTT_INBOX_QUEUE_FAIL;
    }
}

static inline mqtt_request_t* _queue_recieve(TickType_t timeout) {
    mqtt_request_t* item = NULL;
    switch(xQueueReceive(xMQTTInboxQueue, &item, timeout)) {
        case pdPASS:    return item;
        case pdFALSE:   return NULL;
        default:        return NULL;
    }
}

int AddToInboxQueue_WithTimeout(mqtt_request_t *item, TickType_t timeout)    { return _queue_result(item, timeout); }
mqtt_request_t* RecieveFromInboxQueue_WithTimeout(TickType_t timeout)        { return _queue_recieve(timeout); }

int AddToInboxQueue(mqtt_request_t *item)    { return _queue_result(item, portMAX_DELAY);    }
int TryAddToInboxQueue(mqtt_request_t *item) { return _queue_result(item, (TickType_t) 10);  }

mqtt_request_t* RecieveFromInboxQueue(void)      { return _queue_recieve(portMAX_DELAY);     }
mqtt_request_t* TryRecieveFromInboxQueue(void)   { return _queue_recieve((TickType_t) 10);   }

static inline EventBits_t _blockfor_ready(void) {
    return xEventGroupWaitBits(
        get_mqtt_event_group(),
        MQTT_EVENT_SERVICE_RDY,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

static inline EventBits_t _blockfor_terminating(void) {
    return xEventGroupWaitBits(
        get_mqtt_event_group(),
        MQTT_EVENT_TERMINATING,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

void MQTT_Inbox_Task(void *pvParameters)
{
    for(;;)
    {
        
    }
}

int StartMQTTInboxTask(void)
{
    if(xMQTTInboxHandle != NULL) return MQTT_INBOX_ALREADY_INIT;
    if(xMQTTInboxQueue != NULL) {
        Print("MQTT Inbox Init", "WARNING: MQTT Inbox Queue already initialized!");
        Print("MQTT Inbox Init", "Either faulty deconstruction or incorrect initialisation! If this keeps reoccuring, please open an issue on GitHub!");
        vQueueDelete(xMQTTInboxQueue);
    }

    xMQTTInboxQueue = xQueueCreate(
        MQTT_INCOMING_REQUEST_QUEUE_LENGTH, 
        sizeof(mqtt_request_t*)
    );

    if(xMQTTInboxQueue == NULL) {
        Print("MQTT Inbox Init", "ERROR: Could not create MQTT Inbox Queue! Out of memory!!");
        return MQTT_INBOX_MEMORY_ERROR;
    }

    BaseType_t result = xTaskCreate(
        MQTT_Inbox_Task,
        "MQTTInbox",
        MQTT_INBOX_STACK_SIZE,
        NULL,
        MQTT_NODE_TASK_PRIORITY,
        &xMQTTInboxHandle
    );

    switch(result)
    {
        case pdPASS:
            Print("MQTT Inbox Init", "MQTT Inbox Task created successfully!");
            return MQTT_INBOX_TASK_CREATED;

        case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
            Print("MQTT Inbox Init", "Could not allocate memory for MQTT Inbox Task!");
            return MQTT_INBOX_MEMORY_ERROR;

        default:
            Print("MQTT Inbox Init", "Could not create MQTT Inbox Task for unknown reason!");
            return MQTT_INBOX_UNKNOWN_FAIL;
    }
}

void StopMQTTInboxTask(void)
{
    Print("MQTT Inbox Terminator", "Stopping MQTT Inbox Task...");
    if(xMQTTInboxHandle == NULL)
    {
        Print("MQTT Inbox Terminator", "MQTT Inbox Task not running! Ignoring.");
        return;
    }

    vTaskDelete(xMQTTInboxHandle);
    xMQTTInboxHandle = NULL;

    vQueueDelete(xMQTTInboxQueue);
    xMQTTInboxQueue = NULL;

    Print("MQTT Inbox Terminator", "MQTT Inbox Task terminated.");
}