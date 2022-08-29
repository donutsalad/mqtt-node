#include "mqttclient.h"
#include "mqttinterface.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

static TaskHandle_t xMQTTOutboxHandle = NULL;
static QueueHandle_t xMQTTOutboxQueue = NULL;

static inline int _queue_result(mqtt_outgoing_t* item, TickType_t timeout) {
    switch(xQueueSendToBack(xMQTTOutboxQueue, (void*) item, timeout)) {
        case pdPASS:        return MQTT_OUTBOX_QUEUE_OKAY;
        case errQUEUE_FULL: return MQTT_OUTBOX_QUEUE_FULL;
        default:            return MQTT_OUTBOX_QUEUE_FAIL;
    }
}

static inline mqtt_outgoing_t* _queue_recieve(TickType_t timeout) {
    mqtt_outgoing_t* item = NULL;
    switch(xQueueReceive(xMQTTOutboxQueue, &item, timeout)) {
        case pdPASS:    return item;
        case pdFALSE:   return NULL;
        default:        return NULL;
    }
}

int AddToOutboxQueue_WithTimeout(mqtt_outgoing_t *item, TickType_t timeout)    { return _queue_result(item, timeout); }
mqtt_outgoing_t* RecieveFromOutboxQueue_WithTimeout(TickType_t timeout)        { return _queue_recieve(timeout); }

int AddToOutboxQueue(mqtt_outgoing_t *item)    { return _queue_result(item, portMAX_DELAY);    }
int TryAddToOutboxQueue(mqtt_outgoing_t *item) { return _queue_result(item, (TickType_t) 10);  }

mqtt_outgoing_t* RecieveFromOutboxQueue(void)      { return _queue_recieve(portMAX_DELAY);     }
mqtt_outgoing_t* TryRecieveFromOutboxQueue(void)   { return _queue_recieve((TickType_t) 10);   }

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

void MQTT_Outbox_Task(void *pvParameters)
{
    for(;;)
    {
        //Try to dequeue, if we have internet; build topic & message and send, or block until we do.
    }
}

int StartMQTTOutboxTask(void)
{
    if(xMQTTOutboxHandle != NULL) return MQTT_OUTBOX_ALREADY_INIT;
    if(xMQTTOutboxQueue != NULL) {
        Print("MQTT Outbox Init", "WARNING: MQTT Outbox Queue already initialized!");
        Print("MQTT Outbox Init", "Either faulty deconstruction or incorrect initialisation! If this keeps reoccuring, please open an issue on GitHub!");
        vQueueDelete(xMQTTOutboxQueue);
    }

    xMQTTOutboxQueue = xQueueCreate(
        MQTT_INCOMING_REQUEST_QUEUE_LENGTH, 
        sizeof(mqtt_outgoing_t*)
    );

    if(xMQTTOutboxQueue == NULL) {
        Print("MQTT Outbox Init", "ERROR: Could not create MQTT Outbox Queue! Out of memory!!");
        return MQTT_OUTBOX_MEMORY_ERROR;
    }

    BaseType_t result = xTaskCreate(
        MQTT_Outbox_Task,
        "MQTTOutbox",
        MQTT_OUTBOX_STACK_SIZE,
        NULL,
        MQTT_NODE_TASK_PRIORITY,
        &xMQTTOutboxHandle
    );

    switch(result)
    {
        case pdPASS:
            Print("MQTT Outbox Init", "MQTT Outbox Task created successfully!");
            return MQTT_OUTBOX_TASK_CREATED;

        case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
            Print("MQTT Outbox Init", "Could not allocate memory for MQTT Outbox Task!");
            return MQTT_OUTBOX_MEMORY_ERROR;

        default:
            Print("MQTT Outbox Init", "Could not create MQTT Outbox Task for unknown reason!");
            return MQTT_OUTBOX_UNKNOWN_FAIL;
    }
}

void StopMQTTOutboxTask(void)
{
    Print("MQTT Outbox Terminator", "Stopping MQTT Outbox Task...");
    if(xMQTTOutboxHandle == NULL)
    {
        Print("MQTT Outbox Terminator", "MQTT Outbox Task not running! Ignoring.");
        return;
    }

    vTaskDelete(xMQTTOutboxHandle);
    xMQTTOutboxHandle = NULL;

    vQueueDelete(xMQTTOutboxQueue);
    xMQTTOutboxQueue = NULL;

    Print("MQTT Outbox Terminator", "MQTT Outbox Task terminated.");
}