#include "commons.h"

#include <freertos/event_groups.h>

#include <esp_netif.h>
#include <esp_system.h>
#include <esp_event.h>

#include <nvs_init.h>
#include <ap_config.h>

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

    char* ssid = get_wifi_ssid_nullterm_safe();
    char* pass = get_wifi_pass_nullterm_safe();

    printf("DEBUG MESSAGE:\nRecovered WiFi Details are as follows:\nSSID: %s\nPassword: %s\n", 
        ssid, pass
    );

    free(ssid);
    free(pass);

    fflush(stdout);
    return;
}