#include "commons.h"

#include <freertos/event_groups.h>

#include <esp_netif.h>
#include <esp_system.h>
#include <esp_event.h>

#include <nvs_init.h>
#include <ap_config.h>
#include <restore.h>
#include <wifistation.h>

#define START_SYSTEM_SUCCESS    0
#define START_SYSTEM_FAILURE    1

#define GET_CONFIG_SUCCESS      0
#define GET_CONFIG_NVS_FALIURE  1
#define GET_CONFIG_WIFI_FAILURE 2
#define GET_CONFIG_REBOOT_NOW   3
#define GET_CONFIG_FAILURE      4

static void WiFiConnectedCallback(void)
{
    Print("WiFi Connected Callback", "WiFi Connected Callback invoked.");
    if(using_restoration_data() == RESTORATION_USING)
    {
        Print("WiFi Connected Callback", "Restoration is used; validating stored configuration...");
        switch(validate_restore_data())
        {
            case FLAGS_SET_UNCHANGED:
                Print("WiFi Connected Callback", "Restoration data already validated! Ignoring...");
                break;

            case FLAGS_SET_SINGLE:
            case FLAGS_SET_TWEAKED:
                Print("WiFi Connected Callback", "Restoration data validated!");
                break;
            
            case FLAGS_SET_UNHANDLED:
                Print("WiFi Connected Callback", "Warning: Unable to set flags upon attempting restore data validation... If issue persists please check src or open an issue on github!");
                break;
        }
    }
}

static void WiFiDisconnectedHandler(void)
{
    Print("WiFi Disconnected Handler", "WiFi Disconnected Handler starting...");
}

static void WiFiStopHandler(void)
{
    Print("WiFi Stop Handler", "WiFi Stop Handler starting...");
}

static void WiFiExhaustionCallback(void)
{
    Print("WiFi Exhaustion Callback", "WiFi Exhaustion Callback invoked.");
    if(using_restoration_data() == RESTORATION_USING)
    {
        Print("WiFi Exhaustion Callback", "Restoration is used; failing currently stored configuration...");
        switch(failed_with_config())
        {
            case FLAGS_SET_UNCHANGED:
                Print("WiFi Exhaustion Callback", "Restoration data has already failed! Ignoring...");
                break;

            case FLAGS_SET_SINGLE:
            case FLAGS_SET_TWEAKED:
                Print("WiFi Exhaustion Callback", "Restoration data marked as failed!");
                break;
            
            case FLAGS_SET_UNHANDLED:
                Print("WiFi Exhaustion Callback", "Warning: Unable to set flags upon attempting restore data failure marking... If issue persists please check src or open an issue on github!");
                break;
        }
    }
}

static void WiFiBadConfigHandler(void)
{
    Print("WiFi Bad Config Handler", "WiFi Bad Config Handler starting...");
    if(using_restoration_data() == RESTORATION_USING)
    {
        Print("WiFi Bad Config Handler", "Restoration is used; invalidating currently stored configuration...");
        switch(bad_auth_from_config())
        {
            case FLAGS_SET_UNCHANGED:
                Print("WiFi Bad Config Handler", "Restoration data already set as invalid! Unsure how we retried with it then; if this continuously happens please check the src or open a github issue!");
                break;

            case FLAGS_SET_SINGLE:
            case FLAGS_SET_TWEAKED:
                Print("WiFi Bad Config Handler", "Restoration data marked as invalid!");
                break;
            
            case FLAGS_SET_UNHANDLED:
                Print("WiFi Bad Config Handler", "Warning: Unable to set flags upon attempting restore data invalidation... If issue persists please check src or open an issue on github!");
                break;
        }
    }
}

int start_system(void)
{
    Print("System Startup", "Currently there is no system tasks to run, dumping our config for debugging purposes.");
    Print("System Startup", "-----------------------------------------------------");
    wifi_connection_details_t wifi_config;
    mqtt_config_t mqtt_details;
    boot_config_t boot_details;
    
    switch(copy_wifi_details(&wifi_config))
    {
        case WIFI_DETAILS_OK:
            Print("System Startup", "WiFi config retrieved");
            break;

        case WIFI_DETAILS_NULL:
            Print("System Startup", "WiFi config pointer is NULL");
            Print("System Startup", "Startup error occured! Rebooting...");
            esp_restart();
            return START_SYSTEM_FAILURE;

        default:
            Print("System Startup", "WiFi config retrieval failed! Rebooting...");
            esp_restart();
            return START_SYSTEM_FAILURE;
    }

    switch(copy_mqtt_config(&mqtt_details))
    {
        case MQTT_DETAILS_OK:
            Print("System Startup", "MQTT config retrieved");
            break;
        
        case MQTT_DETAILS_NULL:
            Print("System Startup", "MQTT config pointer is NULL");
            Print("System Startup", "Startup error occured! Rebooting...");
            esp_restart();
            return START_SYSTEM_FAILURE;

        default:
            Print("System Startup", "MQTT config retrieval failed! Rebooting...");
            esp_restart();
            return START_SYSTEM_FAILURE;
    }

    switch(copy_boot_config(&boot_details))
    {
        case BOOT_DETAILS_OK:
            Print("System Startup", "Boot config retrieved");
            break;
        
        case BOOT_DETAILS_NULL:
            Print("System Startup", "Boot config pointer is NULL");
            Print("System Startup", "Startup error occured! Rebooting...");
            esp_restart();
            return START_SYSTEM_FAILURE;

        default:
            Print("System Startup", "Boot config retrieval failed! Rebooting...");
            esp_restart();
            return START_SYSTEM_FAILURE;
    }

    printf("DEBUG MESSAGE:\nRecovered WiFi Details are as follows:\nSSID: %s\nPassword: %s\n", wifi_config.ssid, wifi_config.pass);
    printf("DEBUG MESSAGE:\nRecovered MQTT Details are as follows:\nHost: %s\nUsername: %s\n", mqtt_details.address, mqtt_details.username);
    printf("DEBUG MESSAGE:\nRecovered Boot Details are as follows:\nBoot Mode: %d\nBoot Delay: %d\n", boot_details.mode, boot_details.delay);

    Print("System Startup", "-----------------------------------------------------");

    fflush(stdout);

    switch(StartWiFiStation(
        &WiFiConnectedCallback,
        &WiFiDisconnectedHandler,
        &WiFiStopHandler,
        &WiFiExhaustionCallback,
        &WiFiBadConfigHandler
    )) 
    {
        case WIFI_STARTUP_OKAY:
            Print("System Startup", "WiFi Station started successfully");
            break;

        case WIFI_ALREADY_INSTANTIATED:
			return START_SYSTEM_FAILURE;

        case WIFI_STARTUP_FAILED:
			return START_SYSTEM_FAILURE;

        case WIFI_STA_INIT_FAILED:
			return START_SYSTEM_FAILURE;

        case WIFI_CONFIG_BAD:
			return START_SYSTEM_FAILURE;

        case WIFI_CONFIG_NVS_ERR:
			return START_SYSTEM_FAILURE;


        case WIFI_SETUP_INVALID:
			return START_SYSTEM_FAILURE;

        case WIFI_SETUP_MEMORY_ERR:
			return START_SYSTEM_FAILURE;

        case WIFI_SETUP_SETUP_FAILED:
			return START_SYSTEM_FAILURE;

    }

    return START_SYSTEM_SUCCESS;
}

int aquire_config(void)
{
    Print("Config Acquisition", "Acquiring config from NVS...");
    switch(InitialiseAccessPoint())
    {
        case AP_ERR_OK:
            Print("Config Acquisition", "WiFi Setup Completed.");
            return GET_CONFIG_SUCCESS;

        case AP_ERR_NVS_PROBLEM:
            Print("Config Acquisition", "WiFi Setup Failed! NVS problem!");
            return GET_CONFIG_NVS_FALIURE;
            //TODO: Attempt to handle the NVS problem by checking the NVS and reinitialising.

        case AP_ERR_REBOOT:
            Print("Config Acquisition", "WiFi Setup Failed and requested a restart! Rebooting...");
            esp_restart();
            return GET_CONFIG_REBOOT_NOW;

        case AP_ERR_UNSAVABLE:
            Print("Config Acquisition", "WiFi Setup Failed! Unsavable error occured! Aborting startup...");
            return GET_CONFIG_FAILURE;

        default:
            Print("Config Acquisition", "WiFi Setup Failed! Unknown error! Aborting startup...");
            return GET_CONFIG_FAILURE;
    }
}

void app_main(void)
{
    Print("System", "Entered startup routine.");

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

    switch(attempt_restore())
    {
        case ATTEMPT_RESTORE_OK:
            Print("System", "Restore successful!");
            Print("System", "Skipping access point setup, attempting to launch with loaded boot settings...");
        CONFIG_PRESENT:
            switch(start_system())
            {
                case START_SYSTEM_SUCCESS:
                    Print("System", "Startup successful!");
                    Print("System", "Falling out of app_main and allowing system to close startup task.");
                    return;

                case START_SYSTEM_FAILURE: 
                    Print("System", "Startup failed!");
                    //TODO: Cleanup and restart maybe?
                    Print("System", "Falling out of app_main and allowing system to close startup task.");
                    return;
                
                default: 
                    Print("System", "Unhandled error. Falling out of startup routine dirtily...");
                    return;
            }

        case ATTEMPT_RESTORE_NONE:
        NO_CONFIG:
            Print("System", "No config to restore");
            Print("System", "Prompting user to configure boot settings...");
            switch(aquire_config())
            {
                case GET_CONFIG_SUCCESS: 
                    Print("System", "Config acquisition successful!");
                    Print("System", "Attempting to boot with new config...");
                    goto CONFIG_PRESENT;

                case GET_CONFIG_NVS_FALIURE: 
                    Print("System", "Config acquisition failed!");
                    Print("System", "Falling out of app_main and allowing system to close startup task.");
                    return;

                case GET_CONFIG_REBOOT_NOW: 
                    Print("System", "Config acquisition failed!");
                    Print("System", "Acquisition let us know failure could potentially be resolved with a reboot.");
                    Print("System", "Rebooting...");
                    esp_restart(); 
                    return;

                case GET_CONFIG_FAILURE: 
                    Print("System", "Config acquisition failed!");
                    Print("System", "Falling out of app_main and allowing system to close startup task.");
                    return;

                //WIFI here because it's unused.
                case GET_CONFIG_WIFI_FAILURE:
                default: 
                    Print("System", "Config acquisition failed for unknown reason!");
                    Print("System", "Falling out of app_main and allowing system to close startup task.");
                    return;
            }

        case ATTEMPT_RESTORE_NVSERR:
            Print("System", "NVS error during restore");
            Print("System", "Unable to restore config from NVS, assuming no valid data present.");
            goto NO_CONFIG;

        default:
            Print("System", "Unhandled error during restore attempt");
            Print("System", "Unable to restore config from NVS, attempting to startup without config anyway.");
            goto NO_CONFIG;
    }
}