#include "commons.h"

#include <freertos/event_groups.h>

#include <esp_netif.h>
#include <esp_system.h>
#include <esp_event.h>

#include <nvs_init.h>
#include <restore.h>
//#include <ap_config.h> ocurrence in restore.h

void app_main(void)
{
    Print("System", "Booting up");
    
    switch(InitialiseNVS())
    {
        case NVS_INIT_OK:
            Print("System", "NVS ready!");
            break;

        case NVS_INIT_FULL:
            Print("System", "NVS full! Please backup your data then erase and try again. Aborting startup...");
            return;

        case NVS_INIT_PANIC:
            Print("System", "NVS initialisation failed! NVS is required for function... Aborting startup...");
            return;

        default:
            Print("System", "NVS unhandled error! Aborting startup...");
            return;
    }

    switch(attempt_restore())
    {
        case ATTEMPT_RESTORE_OK:
            Print("System", "Restore successful!");
            Print("System", "Skipping access point setup, attempting to launch with loaded boot settings...");
            goto BOOT_CONFIGURED;

        case ATTEMPT_RESTORE_NONE:
            Print("System", "No config to restore");
            Print("System", "Prompting user to configure boot settings...");
            break;

        case ATTEMPT_RESTORE_NVSERR:
            Print("System", "NVS error during restore");
            Print("System", "Unable to restore config from NVS, assuming no valid data present.");
            Print("System", "Prompting user to configure boot settings...");
            break;

        default:
            Print("System", "Unhandled error during restore attempt");
            Print("System", "Unable to restore config from NVS, aborting startup.");
            return;
    }

    switch(esp_netif_init())
    {
        case ESP_OK:
            Print("System", "Network interface initialised");
            break;

        //Peeking the code shows they use size, but state seems more valid.
        //Perhaps it's a typo and will be fixed in the future, keeping both to be safe.
        case ESP_ERR_INVALID_SIZE:
        case ESP_ERR_INVALID_STATE:
            Print("System", "Network interface already initialised");
            Print("System", "Continuing startup anyway...");
            break;

        default:
            Print("System", "Network interface initialisation failed! Aborting startup...");
            return;
    }

    switch(esp_event_loop_create_default())
    {
        case ESP_OK:
            Print("System", "Event loop initialised");
            break;

        case ESP_ERR_INVALID_STATE:
            Print("System", "Event loop already initialised");
            Print("System", "Continuing startup anyway...");
            break;

        case ESP_ERR_NO_MEM:
            Print("System", "Event loop initialisation failed! Out of memory!");
            Print("System", "Fatal error. Aborting startup!!! Please rebuild or flash a different deivce.");
            return;

        default:
            Print("System", "Event loop initialisation failed! Aborting startup...");
            return;
    }

    switch(InitialiseAccessPoint())
    {
        case AP_ERR_OK:
            Print("System", "WiFi Setup Completed.");
            break;

        case AP_ERR_NVS_PROBLEM:
            Print("System", "WiFi Setup Failed! NVS problem!");
            break;
            //TODO: Attempt to handle the NVS problem by checking the NVS and reinitialising.

        case AP_ERR_REBOOT:
            Print("System", "WiFi Setup Failed and requested a restart! Rebooting...");
            esp_restart();
            break;

        case AP_ERR_UNSAVABLE:
            Print("System", "WiFi Setup Failed! Unsavable error occured! Aborting startup...");
            return;

        default:
            Print("System", "WiFi Setup Failed! Unknown error! Aborting startup...");
            return;
    }

    //Ensure everything is cleaned up

    wifi_connection_details_t wifi_config;
    mqtt_config_t mqtt_details;
    boot_config_t boot_details;

BOOT_CONFIGURED:

    switch(copy_wifi_details(&wifi_config))
    {
        case WIFI_DETAILS_OK:
            Print("System", "WiFi config retrieved");
            break;

        case WIFI_DETAILS_NULL:
            Print("System", "WiFi config pointer is NULL");
            Print("System", "Startup error occured! Rebooting...");
            esp_restart();
            return;

        default:
            Print("System", "WiFi config retrieval failed! Rebooting...");
            esp_restart();
            return;
    }

    switch(copy_mqtt_config(&mqtt_details))
    {
        case MQTT_DETAILS_OK:
            Print("System", "MQTT config retrieved");
            break;
        
        case MQTT_DETAILS_NULL:
            Print("System", "MQTT config pointer is NULL");
            Print("System", "Startup error occured! Rebooting...");
            esp_restart();
            return;

        default:
            Print("System", "MQTT config retrieval failed! Rebooting...");
            esp_restart();
            return;
    }

    switch(copy_boot_config(&boot_details))
    {
        case BOOT_DETAILS_OK:
            Print("System", "Boot config retrieved");
            break;
        
        case BOOT_DETAILS_NULL:
            Print("System", "Boot config pointer is NULL");
            Print("System", "Startup error occured! Rebooting...");
            esp_restart();
            return;

        default:
            Print("System", "Boot config retrieval failed! Rebooting...");
            esp_restart();
            return;
    }

    printf("DEBUG MESSAGE:\nRecovered WiFi Details are as follows:\nSSID: %s\nPassword: %s\n", wifi_config.ssid, wifi_config.pass);
    printf("DEBUG MESSAGE:\nRecovered MQTT Details are as follows:\nHost: %s\nUsername: %s\n", mqtt_details.address, mqtt_details.username);
    printf("DEBUG MESSAGE:\nRecovered Boot Details are as follows:\nBoot Mode: %d\nBoot Delay: %d\n", boot_details.mode, boot_details.delay);

    fflush(stdout);
    return;
}