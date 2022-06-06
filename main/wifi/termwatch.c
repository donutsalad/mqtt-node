#include "wifistation.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

static TaskHandle_t xTerminationListener = NULL;

static inline EventBits_t _blockfor_stopping(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_STOPPING,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

void WiFi_Termination_Listener(void *pvParameters)
{
    Print("WiFi Termination Listener", "Blocking task until WiFi stop flag set.");
    _blockfor_stopping();
    Print("WiFi Termination Listener", "WiFi stop flag set. Terminating.");
    xEventGroupClearBits(get_wifi_event_group(), WIFI_STATION_STOPPING | WIFI_STATION_STARTED);
    Termination();
    //Sleep until our task is killed.

    Print("DEBUDEBUDEBDUEBU", "We're suspending the task right now !!!!");
    vTaskSuspend(NULL);
}

void KillWiFiTerminationListener(void)
{
    if (xTerminationListener != NULL) {
        Print("WiFi Termination Listener", "Killing WiFi termination listener.");
        vTaskDelete(xTerminationListener);
        xTerminationListener = NULL;
    }

    Print("WiFi Termination Listener", "WiFi termination listener dead.");
}

int StartWiFiTerminationListener(void)
{
    if(xTerminationListener != NULL) return WIFI_TASK_ALREADY_CREATED;

    BaseType_t result = xTaskCreate(
        WiFi_Termination_Listener,
        "Termination-Listener",
        WIFI_TERMINATION_LISTENER_STACK,
        NULL,
        tskIDLE_PRIORITY,
        &xTerminationListener
    );

    switch(result)
    {
        case pdPASS:
            Print("WiFi Task Init", "Termination Listener Task created successfully!");
            return WIFI_TASK_CREATED;

        case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
            Print("WiFi Task Init", "Could not allocate memory for Termination Listener Task!");
            return WIFI_TASK_MEMORY_FAILURE;

        default:
            Print("WiFi Task Init", "Unknown error whilst creating the Termination Listener Task!");
            return WIFI_TASK_UNKNOWN_FAILURE;
    }
}