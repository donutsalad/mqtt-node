#include "ap_config.h"

#include <string.h>

#include <freertos/event_groups.h>

#include <esp_err.h>
#include <esp_wifi.h>

static wifi_config_t ap_config = {
    .ap = {
        .ssid = AP_SSID,
        .password = AP_PASSWORD,

        .ssid_len = strlen(AP_SSID),

        .authmode = WIFI_AUTH_WPA2_PSK,
        .max_connection = AP_MAXCONNECTIONS,

        .channel = AP_CHANNEL,
        .beacon_interval = AP_BEACON_INTERVAL,
    },
};

static EventGroupHandle_t wifi_event_group;

//TODO: CRITICAL - Handle WEBSERVER_FAIL and default cases during WIFI_EVENT_AP_STACONNECTED. Currently leaves system in a broken state.
//TODO: Efficiency - Shutdown webserver when all clients are disconnected if still waiting for config to save on memory.
static void ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_AP_STACONNECTED:
                Print("AP", "Station connected");
                switch(InitialiseWebserver())
                {
                    case SERVER_OK:
                        Print("AP", "Webserver initialised");
                        break;

                    case SERVER_EXISTS:
                        Print("AP", "Webserver already initialised, ignoring event.");
                        return;

                    //TODO: Handle this by triggering a reboot or by reattempting to initialise the webserver.
                    case SERVER_FATAL:
                        Print("AP", "Webserver failed to initialise!");
                        Print("AP", "This is a fatal unhandeled error currently, if you are reading this please hard reset.");
                        return;

                    default:
                        Print("AP", "Webserver returned unknown error");
                        return;
                }
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                Print("AP", "Station disconnected");
                break;

            default:
                Print("AP", "Unknown event triggered in ap_event_handler.");
                break;
        }
    }
    else
    {
        Print("AP", "Unknown event triggered in ap_event_handler.");
        return;
    }
}

static esp_netif_t* ap_netif;

static int start_ap(void)
{
    wifi_event_group = xEventGroupCreate();
    ap_netif = esp_netif_create_default_wifi_ap();
    if(ap_netif != NULL)
    {
        wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
        switch(esp_wifi_init(&init_config))
        {
            case ESP_OK:
                Print("AP", "WiFi initialised");
                break;

            case ESP_ERR_INVALID_STATE:
                Print("AP", "WiFi already initialised");
                Print("AP", "Continuing startup anyway...");
                break;

            default:
                Print("AP", "WiFi initialisation failed");
                return AP_IERR_UNSAVABLE;
        }

        switch(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &ap_event_handler,
            NULL, NULL
        ))
        {
            case ESP_OK:
                Print("AP", "WiFi event handler registered");
                break;

            case ESP_ERR_INVALID_STATE:
                Print("AP", "WiFi event handler already registered");
                Print("AP", "Unable to attach startup event handler! Aborting startup...");
                return AP_IERR_REBOOT;

            default:
                Print("AP", "WiFi event handler registration failed for unknown reason! Aborting startup...");
                return AP_IERR_UNSAVABLE;
        }

        switch(esp_wifi_set_mode(WIFI_MODE_AP))
        {
            case ESP_OK:
                Print("AP", "WiFi mode set to AP");
                break;

            case ESP_ERR_WIFI_NOT_INIT:
                Print("AP", "WiFi not initialised! Aborting startup...");
                return AP_IERR_WIFI_NOINIT;

            case ESP_ERR_INVALID_ARG:
                Print("AP", "Invalid argument? Unsure whow we got this error... Aborting startup...");
                return AP_IERR_UNSAVABLE;

            default:
                Print("AP", "WiFi mode set to AP failed for unknown reason! Aborting startup...");
                return AP_IERR_UNSAVABLE;
        }

        switch(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config))
        {
            case ESP_OK:
                Print("AP", "WiFi configuration set");
                break;

            case ESP_ERR_WIFI_NOT_INIT:
                Print("AP", "WiFi not initialised! Aborting startup...");
                return AP_IERR_WIFI_NOINIT;

            case ESP_ERR_INVALID_ARG:
                Print("AP", "Invalid argument? Unsure whow we got this error... Aborting startup...");
                return AP_IERR_UNSAVABLE;

            case ESP_ERR_WIFI_IF:
                Print("AP", "Invalid wireless interface? Unsure whow we got this error... Aborting startup...");
                return AP_IERR_UNSAVABLE;

            case ESP_ERR_WIFI_MODE:
                Print("AP", "Invalid WiFi mode? Unsure whow we got this error... Aborting startup...");
                return AP_IERR_UNSAVABLE;

            case ESP_ERR_WIFI_PASSWORD:
                Print("AP", "Invalid WiFi password? Unsure whow we got this error... Aborting startup...");
                return AP_IERR_BADPASSWORD;

            case ESP_ERR_WIFI_NVS:
                Print("AP", "Internal NVS Error in WiFi Stack! Aborting startup...");
                return AP_IERR_NVS_PROBLEM;

            default:
                Print("AP", "WiFi configuration set failed for unknown reason! Aborting startup...");
                return AP_IERR_UNSAVABLE;
        }

        switch(esp_wifi_start())
        {
            case ESP_OK:
                Print("AP", "WiFi started");
                break;

            case ESP_ERR_WIFI_NOT_INIT:
                Print("AP", "WiFi not initialised! Aborting startup...");
                return AP_IERR_WIFI_NOINIT;

            case ESP_ERR_INVALID_ARG:
                Print("AP", "Invalid argument? Unsure whow we got this error... Aborting startup...");
                return AP_IERR_UNSAVABLE;

            case ESP_ERR_NO_MEM:
                Print("AP", "No memory available! Aborting startup...");
                return AP_IERR_NO_MEM;

            case ESP_ERR_WIFI_CONN:
                Print("AP", "WiFi connection failed! Aborting startup...");
                return AP_IERR_UNSAVABLE;

            case ESP_FAIL:
                Print("AP", "WiFi start failed from a unknown internal WiFi stack error! Aborting startup...");
                return AP_IERR_UNSAVABLE;

            default:
                Print("AP", "WiFi start failed for unknown reason! Aborting startup...");
                return AP_IERR_UNSAVABLE;
        }
        return AP_IERR_OK;
    }
    else
    {
        Print("AP", "Failed to create network interface");
        return AP_IERR_UNSAVABLE;
    }
}

static int stop_ap(void)
{
    if(wifi_event_group != NULL)
    {
        wifi_event_group = NULL;
    }

    if(ap_netif != NULL)
    {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }

    switch(esp_wifi_stop())
    {
        case ESP_OK:
            Print("AP", "WiFi stopped");
            break;

        case ESP_ERR_WIFI_NOT_INIT:
            Print("AP", "WiFi not initialised! Please check src for descrepencies, however since wifi is stopped we can continue anyway...");
            return AP_IERR_WIFI_NOINIT;

        default:
            Print("AP", "WiFi stop failed for unknown reason! Aborting shutdown...");
            return AP_IERR_UNSAVABLE;
    }

    return AP_IERR_OK;
}

int InitialiseAccessPoint(void)
{
    switch(start_ap())
    {
        case AP_IERR_OK:
            Print("AP", "Access Point started!");
            break;

        case AP_IERR_REBOOT:
            Print("AP", "Access Point failed to start! Requesting a reboot...");
            return AP_ERR_REBOOT;

        case AP_IERR_WIFI_NOINIT:
            Print("AP", "Access Point unable to be initialised! Aborting startup...");
            return AP_ERR_UNSAVABLE;
            //Maybe at some point fixes should be attempted?

        case AP_IERR_BADPASSWORD:
            Print("AP", "Invalid WiFi password! Aborting startup...");
            Print("AP", "Please check the password in the config file! If error occurs with the default password, please submit a github issue!");
            return AP_ERR_UNSAVABLE;
            //Eventually when NVS stores a custom password, this will fallback to the default password.
            //For now however the only potential issue is fatal - in the definitions.

        case AP_IERR_NVS_PROBLEM:
            Print("AP", "Internal NVS Error in WiFi Stack! Aborting startup...");
            return AP_ERR_NVS_PROBLEM;


        case AP_IERR_UNSAVABLE:
            Print("AP", "Unable to start Access Point! Aborting startup...");
            return AP_ERR_UNSAVABLE;
        default:
            Print("AP", "Unknown error! Aborting startup...");
            return AP_ERR_UNSAVABLE;
    }

    SetupWebserverEventListeners();
    wait_for_wifi_config();
    switch(TriggerWebserverClose())
    {
        case SERVER_OK:
            Print("AP", "Webserver closed");
            break;

        case SERVER_IGNORE:
            Print("AP", "Non fatal warning occured, however webserver shutdown was successful. Continuing startup...");
            break;
        
        case SERVER_FATAL:
            Print("AP", "Webserver failed to close! Aborting startup...");
            return AP_ERR_UNSAVABLE;
            //Maybe at some point fixes should be attempted? Force Shutdown?

        default:
            Print("AP", "Unknown error! Aborting startup...");
            return AP_ERR_UNSAVABLE;
    }

    wait_for_shutdown_completion();
    switch(stop_ap())
    {
        case AP_IERR_OK:
            Print("AP", "Access Point WiFi stack stopped!");
            break;

        case AP_IERR_WIFI_NOINIT:
            Print("AP", "Access Point was already stopped? Unsure if this is safe, check related src if other issues crop up. Continuing startup...");
            break;

        case AP_IERR_UNSAVABLE:
            Print("AP", "Unable to stop Access Point! Aborting shutdown...");
            return AP_ERR_UNSAVABLE;

        default:
            Print("AP", "Unknown error! Aborting shutdown...");
            return AP_ERR_UNSAVABLE;
    }

    Print("AP", "WiFi detail aquisition completed!");
    return AP_ERR_OK;
}