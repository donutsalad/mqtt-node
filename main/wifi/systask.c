#include "wifistation.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include <esp_wifi.h>

static TaskHandle_t xWiFiHandle = NULL;

static uint8_t innocent_retry_count = 0;
static uint8_t exhausation_counter  = 0;
static uint8_t unknown_counter      = 0;

static uint8_t connectionvalid      = 0;

static inline EventBits_t _blockfor_started(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_STARTED,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

static inline EventBits_t _blockfor_connected(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_CONNECTED,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

static inline EventBits_t _blockfor_disconnected(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_DISCONNECTED,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

static inline EventBits_t _blockfor_connectionevent(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_CONNECTED | WIFI_STATION_DISCONNECTED,
        pdFALSE, pdFALSE,
        portMAX_DELAY
    );
}

static inline EventBits_t _blockfor_stopping(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_STOPPING,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

void WiFi_Background_Task(void *pvParameters)
{
    Print("WiFi Task", "Blocking task until WiFi start flag set.");
    _blockfor_started();
    Print("WiFi Task", "WiFi start flag set. Starting.");
    esp_wifi_connect();
    for(;;)
    {
        EventBits_t bits = _blockfor_connectionevent();
        if(bits & WIFI_STATION_CONNECTED && bits & WIFI_STATION_DISCONNECTED)
        {
            Print("WiFi Task", "Invalid state! WiFi connected and disconnected simultaneously.");
            xEventGroupClearBits(get_wifi_event_group(), WIFI_STATION_CONNECTED | WIFI_STATION_DISCONNECTED);
            InvalidDuality();
        }
        else if(bits & WIFI_STATION_CONNECTED)
        {
            xEventGroupClearBits(get_wifi_event_group(), WIFI_STATION_CONNECTED | WIFI_STATION_DISCONNECTED);
            Print("WiFi Task", "WiFi connected.");
            Connected();
            connectionvalid = 1; //TODO: Maybe handle already 1?
        }
        else if(bits & WIFI_STATION_DISCONNECTED)
        {
            xEventGroupClearBits(get_wifi_event_group(), WIFI_STATION_DISCONNECTED | WIFI_STATION_CONNECTED);
            Print("WiFi Task", "WiFi disconnection event.");
            switch(DisconnectionEntailment())
            {
                case DISCONNECT_RESPONCE_RETRY:
                    if(innocent_retry_count > RETRY_MAX_INNOCENT)
                    {
                        Print("WiFi Task", "Retry count exceeded.");
                        InnocentExhaustion();
                        break;
                    }
                    innocent_retry_count++;
                    if(connectionvalid == 1) {
                        Print("WiFi Task", "Connection was valid.");
                        Print("WiFi Task", "Calling the disconnection handler.");
                        connectionvalid = 0;
                        Disconnected();
                    }
                    Print("WiFi Task", "Retrying connection.");
                    esp_wifi_connect();
                    break;

                case DISCONNECT_RESPONCE_RETRYWAIT:
                    if(innocent_retry_count > RETRY_MAX_INNOCENT)
                    {
                        Print("WiFi Task", "Retry count exceeded.");
                        InnocentExhaustion();
                        break;
                    }
                    innocent_retry_count++;
                    Print("WiFi Task", "Waiting a few seconds before retrying connection.");
                    vTaskDelay(DISCONNECT_RESPONCE_RETRY_TIMEOUT_MS / portTICK_PERIOD_MS);
                    if(connectionvalid == 1) {
                        Print("WiFi Task", "Connection was valid.");
                        Print("WiFi Task", "Calling the disconnection handler.");
                        connectionvalid = 0;
                        Disconnected();
                    }
                    Print("WiFi Task", "Retrying connection.");
                    esp_wifi_connect();
                    break;

                case DISCONNECT_RESPONCE_NONEFOUND:
                    if(exhausation_counter > RETRY_MAX_EXHAUSTED)
                    {
                        Print("WiFi Task", "Retry count exceeded.");
                        Exhaustion();
                        break;
                    }
                    exhausation_counter++;
                    Print("WiFi Task", "No AP found. Waiting a few moments before retrying.");
                    vTaskDelay(DISCONNECT_RESPONCE_NONEFOUND_TIMEOUT_MS / portTICK_PERIOD_MS);
                    if(connectionvalid == 1) {
                        Print("WiFi Task", "Connection was valid.");
                        Print("WiFi Task", "Calling the disconnection handler.");
                        connectionvalid = 0;
                        Disconnected();
                    }
                    Print("WiFi Task", "Retrying connection.");
                    esp_wifi_connect();
                    break;

                case DISCONNECT_RESPONCE_REJECT:
                    Print("WiFi Task", "Connection rejected on ground of bad auth, letting the system know to prompt user again.");
                    Reject();
                    break;

                case DISCONNECT_RESPONCE_UNKNOWN:
                    if(unknown_counter > RETRY_MAX_UNKNOWN)
                    {
                        Print("WiFi Task", "Retry count exceeded.");
                        UnknownExhaustion();
                        break;
                    }
                    exhausation_counter++;
                    Print("WiFi Task", "Unknown disconnection response. Treating this as a sign of bad input.");
                    Print("WiFi Task", "Waiting a few moment and retrying again anyway.");
                    vTaskDelay(DISCONNECT_RESPONCE_NONEFOUND_TIMEOUT_MS / portTICK_PERIOD_MS);
                    if(connectionvalid == 1) {
                        Print("WiFi Task", "Connection was valid.");
                        Print("WiFi Task", "Calling the disconnection handler.");
                        connectionvalid = 0;
                        Disconnected();
                    }
                    Print("WiFi Task", "Retrying connection.");
                    esp_wifi_connect();
                    break;
            }
        }
    }
}

int StartWiFiTask(void)
{
    if(xWiFiHandle != NULL) return WIFI_TASK_ALREADY_CREATED;

    BaseType_t result = xTaskCreate(
        WiFi_Background_Task,
        "WiFi-Background",
        WIFI_BACKGROUND_TASK_STACK,
        NULL,
        tskIDLE_PRIORITY,
        &xWiFiHandle
    );
    
    switch(result)
    {
        case pdPASS:
            Print("WiFi Task Init", "WiFi Task created successfully!");
            return WIFI_TASK_CREATED;
            
        case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
            Print("WiFi Task Init", "Could not allocate memory for WiFi Task!");
            return WIFI_TASK_MEMORY_FAILURE;

        default:
            Print("WiFi Task Init", "Unknown error whilst creating the WiFi Task!");
            return WIFI_TASK_UNKNOWN_FAILURE;
    }
}

void StopWiFiTask(void)
{
    Print("WiFi Task Terminator", "WiFi Task termination requested.");
    if(xWiFiHandle == NULL)
    {
        Print("WiFi Task Terminator", "WiFi Task not running! Ignoring.");
        return;
    }
    vTaskDelete(xWiFiHandle);
    Print("WiFi Task Terminator", "WiFi Task terminated.");
    xWiFiHandle = NULL;
}