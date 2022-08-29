#include "commons.h"

#include <freertos/event_groups.h>

#include <esp_netif.h>
#include <esp_system.h>
#include <esp_event.h>

#include <nvs_init.h>
#include <ap_config.h>
#include <restore.h>
#include <wifistation.h>
#include <mqttclient.h>

#define START_SYSTEM_SUCCESS    0
#define START_SYSTEM_FAILURE    1

#define GET_CONFIG_SUCCESS      0
#define GET_CONFIG_NVS_FALIURE  1
#define GET_CONFIG_WIFI_FAILURE 2
#define GET_CONFIG_REBOOT_NOW   3
#define GET_CONFIG_FAILURE      4

#define REPROMPT_STACK_SIZE     3072

#define REPROMPT_SUCCESS        0
#define REPROMPT_FAILURE        1
#define REPROMPT_START_FAILURE  2

#define REPROMPT_TASK_CREATED   0
#define REPROMPT_TASK_EXISTS    1
#define REPROMPT_TASK_ERROR     2
#define REPROMPT_TASK_NOMEM     3

#define REPROMPT_STOP_SUCCESS   0
#define REPROMPT_STOP_FAILURE   1
#define REPROMPT_STOP_NONE      2

int aquire_config(void);
int start_system(void);

static inline int _reprompt_user(void)
{
    Print("System", "Reprompting the user for configuration details!");
    switch(aquire_config())
    {
        case GET_CONFIG_SUCCESS: 
            Print("System", "Config acquisition successful!");
            Print("System", "Attempting to boot with new config...");
            break;

        case GET_CONFIG_FAILURE: 
        case GET_CONFIG_NVS_FALIURE: 
            Print("System", "Config acquisition failed!");
            return REPROMPT_FAILURE;

        case GET_CONFIG_REBOOT_NOW: 
            Print("System", "Config acquisition failed!");
            Print("System", "Acquisition let us know failure could potentially be resolved with a reboot.");
            Print("System", "Rebooting...");
            esp_restart(); 
            return REPROMPT_FAILURE; //We shouldn't get here x3

        //WIFI here because it's unused.
        case GET_CONFIG_WIFI_FAILURE:
        default: 
            Print("System", "Config acquisition failed for unknown reason!");
            return REPROMPT_FAILURE;
    }

    Print("System", "Attempting to relaunch the system!");
    switch(start_system())
    {
        case START_SYSTEM_SUCCESS:
            Print("System", "Startup successful!");
            return REPROMPT_SUCCESS;

        case START_SYSTEM_FAILURE: 
            Print("System", "Startup failed!");
            return REPROMPT_START_FAILURE;
        
        default: 
            Print("System", "Unhandled error. Falling out of startup routine dirtily...");
            return REPROMPT_START_FAILURE;
    }
}

//TODO: Maximum retries before booting.
static TaskHandle_t _reprompt_task_handle;
static void Reprompt_Task(void* pvParameters)
{
    //Not always but in this case;
    RECURSION_IS_BAD:
    switch(_reprompt_user())
    {
        case REPROMPT_SUCCESS:
            Print("System", "Reprompting user successful!");
            Print("System", "Exiting reprompt task...");
            break;

        //TODO: Implement a maximum retry counter on this case.
        case REPROMPT_FAILURE:
            Print("System", "Reprompting user failed!");
            Print("System", "Attempting to reprompt the user again...");
            goto RECURSION_IS_BAD; // - Because it increments the stack pointer! [goto] is clean code! Fight me!

        case REPROMPT_START_FAILURE:
            Print("System", "Starting after reprompt failed!");
            Print("System", "Attempting to reprompt the user again, it could've been an issue with the configuration...");
            goto RECURSION_IS_BAD; // - When we only have a few kilobytes for our entire task!

        default:
            Print("System", "Unhandled error. Exiting reprompt task...");
            Print("System", "This error message shouldn't occur! Please check the src or submit an issue on github!");
            break;
    }

    _reprompt_task_handle = NULL;
    vTaskDelete(NULL);
}

static int LaunchRepromptTask(void)
{
    Print("System", "Request for reprompt task launch received!");

    if(_reprompt_task_handle != NULL)
    {
        Print("System", "Reprompt task already exists!");
        Print("System", "Ignoring request to remprompt user - unsure how duplicate requests formed! Please check src or open a github issue!");
        return REPROMPT_TASK_EXISTS;
    }

    Print("System", "Creating reprompt task...");
    BaseType_t result = xTaskCreate(
        Reprompt_Task,
        "Reprompt-User",
        REPROMPT_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY,
        &_reprompt_task_handle
    );

    switch(result)
    {
        case pdPASS:
            Print("System", "Reprompt task created successfully!");
            return REPROMPT_TASK_CREATED;

        case errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY:
            Print("System", "Reprompt task creation failed!");
            Print("System", "Could not allocate memory for reprompt task!");
            return REPROMPT_TASK_NOMEM;

        default:
            Print("System", "Reprompt task creation failed!");
            return REPROMPT_TASK_ERROR;
    }
}

static int KillRepromptTask(void)
{
    Print("System", "Request for reprompt task kill received!");
    if(_reprompt_task_handle == NULL)
    {
        Print("System", "Reprompt task does not exist!");
        return REPROMPT_STOP_NONE;
    }

    //TODO: Gracefully stop WiFi stack and any consequential processes that might be unclean during exit.

    Print("System", "Killing reprompt task...");
    vTaskDelete(_reprompt_task_handle);
    Print("System", "Remprompt task killed successfully!");
    _reprompt_task_handle = NULL;
    return REPROMPT_STOP_SUCCESS;
}

static void MQTTSuccessfulCallback(void)
{
    //Validate and exit.

    Print("MQTT Successful Callback", "Startup routine completed!");

    if(using_restoration_data() == RESTORATION_USING)
    {
        Print("MQTT Successful Callback", "Restoration is used; validating stored configuration...");
        switch(validate_restore_data())
        {
            case FLAGS_SET_UNCHANGED:
                Print("MQTT Successful Callback", "Restoration data already validated! Ignoring...");
                break;

            case FLAGS_SET_SINGLE:
            case FLAGS_SET_TWEAKED:
                Print("MQTT Successful Callback", "Restoration data validated!");
                break;
            
            case FLAGS_SET_UNHANDLED:
                Print("MQTT Successful Callback", "Warning: Unable to set flags upon attempting restore data validation... If issue persists please check src or open an issue on github!");
                break;
        }
    }

    Print("MQTT Successful Callback", "Exiting callback, system tasks will self-manage from here-on out.");
    return;
}

static void MQTTInvalidCallback(void)
{
    Print("MQTT Bad Config Handler", "Since the MQTT details were invalid, we're going to try to reprompt the user and try again.");
    switch(LaunchRepromptTask())
    {
        case REPROMPT_TASK_CREATED:
            Print("MQTT Bad Config Handler", "Reprompt task created successfully!");
            break;

        case REPROMPT_TASK_NOMEM:
            Print("MQTT Bad Config Handler", "Reprompt task creation failed!");
            Print("MQTT Bad Config Handler", "Could not allocate memory for reprompt task!");
            break;

        case REPROMPT_TASK_ERROR:
        case REPROMPT_TASK_EXISTS:
        default:
            Print("MQTT Bad Config Handler", "Reprompt task creation failed!");
            Print("MQTT Bad Config Handler", "Unhandled error occurred while creating reprompt task!");
            break;
    }
}

static void MQTTUngracefulShutdown(void)
{
    Print("MQTT Error Post-Mortem", "Ungraceful shutdown occured. Rebooting...");
    esp_restart();
}

static void WiFiConnectedCallback(void)
{
    Print("WiFi Connected Callback", "WiFi Connected Callback invoked.");

/*----------------------------------------------------------------
 * Config validation moved to post MQTT Validation
 *----------------------------------------------------------------
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
*/

    Print("WiFi Connected Callback", "Launching MQTT Service!");
    switch(StartMQTTClient(
        &MQTTSuccessfulCallback, 
        &MQTTInvalidCallback, 
        &MQTTUngracefulShutdown
    )) {
        case MQTT_START_SUCCESS:
            //Nothing to do but wait.
            Print("WiFi Connected Callback", "MQTT Client Startup initiated.");
            break;

        case MQTT_START_DUPLICATE:
            //WRONG: Already started -> send the resume signal.
            //We can block for hasIP inside the MQTT client.
            Print("WiFi Connected Callback", "MQTT Client already started! Must be a reconnection, Ignoring...");
            break;

        case MQTT_START_FAILURE:
            //Handle failure and reboot.
            Print("WiFi Connected Callback", "MQTT Client unable to be started! Rebooting...");
            esp_restart();
            //TODO: Handle the faliure better.
            return;

        default:
            //Same as MQTT START FAILURE except log unknown return code.
            Print("WiFi Connected Callback", "MQTT Client returned an unknown responce! Treating this as an error and rebooting, please check src for possible codes!");
            esp_restart();
            return;
    }
}

static void WiFiDisconnectedHandler(void)
{
    Print("WiFi Disconnected Handler", "WiFi Disconnected Handler starting...");
    //WRONG: Let the MQTT service know to wait up.
    //MQTT Can handle the disconnection waiting for HAS_IP itself.
    Print("WiFi Disconnected Handler", "Nothing to do, ignoring...");
}

static void WiFiStopHandler(void)
{
    Print("WiFi Stop Handler", "WiFi Stopped, terminating MQTT services and aborting...");
    ShutdownMQTTServices();
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

            default:
                Print("WiFi Exhaustion Callback", "Warning: Unhandled error occurred while attempting to mark restoration data as failed! If issue persists please check src or open an issue on github!");
                break;
        }
    }

    Print("WiFi Exhaustion Callback", "Since we can't seem to connect to the WiFi network, we're going to try to reprompt the user and try again.");
    switch(LaunchRepromptTask())
    {
        case REPROMPT_TASK_CREATED:
            Print("WiFi Exhaustion Callback", "Reprompt task created successfully!");
            break;

        case REPROMPT_TASK_NOMEM:
            Print("WiFi Exhaustion Callback", "Reprompt task creation failed!");
            Print("WiFi Exhaustion Callback", "Could not allocate memory for reprompt task!");
            break;

        case REPROMPT_TASK_ERROR:
        case REPROMPT_TASK_EXISTS:
        default:
            Print("WiFi Exhaustion Callback", "Reprompt task creation failed!");
            Print("WiFi Exhaustion Callback", "Unhandled error occurred while creating reprompt task!");
            break;
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

    Print("WiFi Bad Config Handler", "Since the network details were invalid, we're going to try to reprompt the user and try again.");
    switch(LaunchRepromptTask())
    {
        case REPROMPT_TASK_CREATED:
            Print("WiFi Bad Config Handler", "Reprompt task created successfully!");
            break;

        case REPROMPT_TASK_NOMEM:
            Print("WiFi Bad Config Handler", "Reprompt task creation failed!");
            Print("WiFi Bad Config Handler", "Could not allocate memory for reprompt task!");
            break;

        case REPROMPT_TASK_ERROR:
        case REPROMPT_TASK_EXISTS:
        default:
            Print("WiFi Bad Config Handler", "Reprompt task creation failed!");
            Print("WiFi Bad Config Handler", "Unhandled error occurred while creating reprompt task!");
            break;
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

    //TODO: We're failing here for some reason!?!?!
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

        case ATTEMPT_RESTORE_NEW_WIFI:
            Print("System", "Even though the configuration was validated in the past, the WiFi credentials have changed.");
            Print("System", "Treating as though no config was present.");
            goto NO_CONFIG;

        case ATTEMPT_RESTORE_BAD_WIFI:
            Print("System", "The WiFi credentials are invalid.");
            Print("System", "Treating as though no config was present.");
            goto NO_CONFIG;

        case ATTEMPT_RESTORE_NEW_MQTT:
            Print("System", "Even though the configuration was validated in the past, the MQTT connection was since invalidated.");
            Print("System", "Treating as though no config was present.");
            goto NO_CONFIG;

        case ATTEMPT_RESTORE_BAD_MQTT:
            Print("System", "The MQTT credentials are invalid.");
            Print("System", "Treating as though no config was present.");
            goto NO_CONFIG;

        default:
            Print("System", "Unhandled error during restore attempt");
            Print("System", "Unable to restore config from NVS, attempting to startup without config anyway.");
            goto NO_CONFIG;
    }
}