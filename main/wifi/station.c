#include "wifistation.h"

#include <stdbool.h>
#include <string.h>

#include <esp_wifi.h>

#include <lwip/err.h>
#include <lwip/sys.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>

#include "ap_config.h"

static esp_netif_t *sta_netif = NULL;
static EventGroupHandle_t wifi_event_group = NULL;

EventGroupHandle_t get_wifi_event_group(void) { return wifi_event_group; }

//https://www.esp32.com/viewtopic.php?t=349
static uint8_t disconnection_reason_buffer = 255;

uint8_t get_disconnection_reason(void) { return disconnection_reason_buffer; }

static struct WiFi_EventHandlers {
    esp_event_handler_instance_t wifi;
    esp_event_handler_instance_t ip;
} wifi_handles;

static struct WiFi_IPdata {
    uint8_t valid;
    esp_ip4_addr_t ip;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gateway;
} wifi_ip_data;

static struct WiFi_Callbacks {
    void (*callback)(void);
    void (*disconnection_handler)(void);
    void (*stop_handler)(void);
    void (*exhaustion_callback)(void);
    void (*bad_config_callback)(void);
} wifi_callbacks;

static wifi_config_t wifi_client_config = {
	.sta = {
		.ssid = "",
		.password = "",
		.threshold.authmode = WIFI_AUTH_WPA2_PSK,
		.pmf_cfg = {
			.capable = true,
			.required = false
		},
	},
};

static inline void _generate_wifi_client_configuration(void) {
    memset(wifi_client_config.sta.ssid,     '\0', sizeof(wifi_client_config.sta.ssid));
    memset(wifi_client_config.sta.password, '\0', sizeof(wifi_client_config.sta.password));

    memcpy(wifi_client_config.sta.ssid,     wifi_details()->ssid, sizeof(wifi_client_config.sta.ssid));
    memcpy(wifi_client_config.sta.password, wifi_details()->pass, sizeof(wifi_client_config.sta.password));
}

static void ClearData(void)
{
    vEventGroupDelete(wifi_event_group);
    esp_netif_destroy(sta_netif);
}

static void _wifi_event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data
) {
    if(event_base != WIFI_EVENT) return;
    switch(event_id)
    {
        case WIFI_EVENT_STA_START:
            xEventGroupSetBits(wifi_event_group, WIFI_STATION_STARTED);
            break;

        case WIFI_EVENT_STA_CONNECTED:
            xEventGroupSetBits(wifi_event_group, WIFI_STATION_CONNECTED);
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            disconnection_reason_buffer = ((wifi_event_sta_disconnected_t*)event_data)->reason;
            xEventGroupSetBits(wifi_event_group, WIFI_STATION_DISCONNECTED);
            break;

        case WIFI_EVENT_STA_STOP:
            xEventGroupSetBits(wifi_event_group, WIFI_STATION_STOPPING);
            break;
    }
}

static void _ip_event_handler(
    void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data
) {
    if(event_base != IP_EVENT) return;
    switch(event_id)
    {
        case IP_EVENT_STA_GOT_IP:
            wifi_ip_data.valid = 1;
            memcpy(&wifi_ip_data.ip,        &((ip_event_got_ip_t*)event_data)->ip_info.ip,      sizeof(wifi_ip_data.ip));
            memcpy(&wifi_ip_data.netmask,   &((ip_event_got_ip_t*)event_data)->ip_info.netmask, sizeof(wifi_ip_data.netmask));
            memcpy(&wifi_ip_data.gateway,   &((ip_event_got_ip_t*)event_data)->ip_info.gw,      sizeof(wifi_ip_data.gateway));
            xEventGroupSetBits(wifi_event_group, WIFI_STATION_HAS_IP);
            break;

        case IP_EVENT_STA_LOST_IP:
            memset(&wifi_ip_data, 0, sizeof(wifi_ip_data));
            xEventGroupSetBits(wifi_event_group, WIFI_STATION_LOST_IP);
            break;
    }
}

int DisconnectionEntailment(void)
{
    switch(disconnection_reason_buffer)
    {
        case WIFI_DISCONNECTION_UNSPECIFIED:
        case WIFI_DISCONNECTION_AUTH_EXPIRE:
        case WIFI_DISCONNECTION_AUTH_LEAVE:
        case WIFI_DISCONNECTION_ASSOC_EXPIRE:
        case WIFI_DISCONNECTION_ASSOC_TOOMANY:
        case WIFI_DISCONNECTION_NOT_AUTHED:
        case WIFI_DISCONNECTION_NOT_ASSOCED:
        case WIFI_DISCONNECTION_ASSOC_LEAVE:
        case WIFI_DISCONNECTION_ASSOC_NOT_AUTHED:
        case WIFI_DISCONNECTION_DISASSOC_PWRCAP_BAD:
        case WIFI_DISCONNECTION_DISASSOC_SUPCHAN_BAD:
        case WIFI_DISCONNECTION_IE_INVALID:
        case WIFI_DISCONNECTION_MIC_FAILURE:
        case WIFI_DISCONNECTION_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_DISCONNECTION_GROUP_KEY_UPDATE_TIMEOUT:
        case WIFI_DISCONNECTION_IE_IN_4WAY_DIFFERS:
        case WIFI_DISCONNECTION_GROUP_CIPHER_INVALID:
        case WIFI_DISCONNECTION_PAIRWISE_CIPHER_INVALID:
        case WIFI_DISCONNECTION_AKMP_INVALID:
        case WIFI_DISCONNECTION_UNSUPP_RSN_IE_VERSION:
        case WIFI_DISCONNECTION_INVALID_RSN_IE_CAP:
        case WIFI_DISCONNECTION_802_1X_AUTH_FAILED:
        case WIFI_DISCONNECTION_CIPHER_SUITE_REJECTED:

            return DISCONNECT_RESPONCE_RETRY;

        case WIFI_DISCONNECTION_BEACON_TIMEOUT:
        case WIFI_DISCONNECTION_ASSOC_FAIL:
        case WIFI_DISCONNECTION_HANDSHAKE_TIMEOUT:

            return DISCONNECT_RESPONCE_RETRYWAIT;


        case WIFI_DISCONNECTION_AUTH_FAIL:
            return DISCONNECT_RESPONCE_REJECT;

        case WIFI_DISCONNECTION_NO_AP_FOUND:
            return DISCONNECT_RESPONCE_NONEFOUND;

        default: return DISCONNECT_RESPONCE_UNKNOWN;
    }
}

int ConnectWithGlobalConfig(void)
{
    if(wifi_event_group != NULL || sta_netif != NULL) return WIFI_ALREADY_INSTANTIATED;

    wifi_event_group = xEventGroupCreate();
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t iconf = WIFI_INIT_CONFIG_DEFAULT();
    switch(esp_wifi_init(&iconf))
    {
        case ESP_OK:
            Print("WiFi Station", "WiFi initialised");
            break;

        case ESP_ERR_NO_MEM:
            Print("WiFi Station", "WiFi initialisation failed! Not enough memory!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        default:
            Print("WiFi Station", "WiFi initialisation failed for unknown reason!");
            ClearData();
            return WIFI_STARTUP_FAILED;
    }

    switch(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &_wifi_event_handler, NULL, &wifi_handles.wifi
    )) {
        case ESP_OK:
            Print("WiFi Station", "WiFi event handler registered");
            break;

        case ESP_ERR_NO_MEM:
            Print("WiFi Station", "WiFi event handler registration failed! Not enough memory!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        case ESP_ERR_INVALID_ARG:
            Print("WiFi Station", "WiFi event handler registration failed! Invalid argument!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        default:
            Print("WiFi Station", "WiFi event handler registration failed for unknown reason!");
            ClearData();
            return WIFI_STARTUP_FAILED;
    }

    switch(esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID,
        &_ip_event_handler, NULL, &wifi_handles.ip
    )) {
        case ESP_OK:
            Print("WiFi Station", "IP event handler registered");
            break;

        case ESP_ERR_NO_MEM:
            Print("WiFi Station", "IP event handler registration failed! Not enough memory!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        case ESP_ERR_INVALID_ARG:
            Print("WiFi Station", "IP event handler registration failed! Invalid argument!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        default:
            Print("WiFi Station", "IP event handler registration failed for unknown reason!");
            ClearData();
            return WIFI_STARTUP_FAILED;
    }
    
    switch(esp_wifi_set_mode(WIFI_MODE_STA))
    {
        case ESP_OK:
            Print("WiFi Station", "WiFi mode set to STA");
            break;

        case ESP_ERR_WIFI_NOT_INIT:
            Print("WiFi Station", "WiFi mode set failed! WiFi not initialised!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        case ESP_ERR_INVALID_ARG:
            Print("WiFi Station", "WiFi mode set failed! Invalid argument!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        default:
            Print("WiFi Station", "WiFi mode set failed for unknown reason!");
            ClearData();
            return WIFI_STARTUP_FAILED;
    }

    _generate_wifi_client_configuration();
    switch(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_client_config))
    {
        case ESP_OK:
            Print("WiFi Station", "WiFi client configuration set");
            break;

        case ESP_ERR_WIFI_NOT_INIT:
            Print("WiFi Station", "WiFi client configuration set failed! WiFi not initialised!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        case ESP_ERR_INVALID_ARG:
            Print("WiFi Station", "WiFi client configuration set failed! Invalid argument!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        case ESP_ERR_WIFI_IF:
            Print("WiFi Station", "WiFi client configuration set failed! Invalid interface!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_CONFIG_BAD;

        case ESP_ERR_WIFI_MODE:
            Print("WiFi Station", "WiFi client configuration set failed! Invalid mode!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_CONFIG_BAD;

        case ESP_ERR_WIFI_PASSWORD:
            Print("WiFi Station", "WiFi client configuration set failed! Invalid password (not wrong password)!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_CONFIG_BAD;

        case ESP_ERR_WIFI_NVS:
            Print("WiFi Station", "WiFi client configuration set failed! NVS error!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_CONFIG_NVS_ERR;

        default:
            Print("WiFi Station", "WiFi client configuration set failed for unknown reason!");
            ClearData();
            return WIFI_STARTUP_FAILED;
    }

    switch(esp_wifi_start())
    {
        case ESP_OK:
            Print("WiFi Station", "WiFi started");
            break;

        case ESP_ERR_WIFI_NOT_INIT:
            Print("WiFi Station", "WiFi start failed! WiFi not initialised!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        case ESP_ERR_INVALID_ARG:
            Print("WiFi Station", "WiFi start failed! Invalid argument!");
            Print("WiFi Station", "Please check src or open a github issue!");
            ClearData();
            return WIFI_STA_INIT_FAILED;

        case ESP_ERR_NO_MEM:
            Print("WiFi Station", "WiFi start failed! Not enough memory!");
            ClearData();
            return WIFI_STARTUP_FAILED;

        case ESP_ERR_WIFI_CONN:
            Print("WiFi Station", "WiFi start failed! Connection error!");
            ClearData();
            return WIFI_STARTUP_FAILED;

        case ESP_FAIL:
            Print("WiFi Station", "WiFi start failed! Internal WiFi error!");
            ClearData();
            return WIFI_STARTUP_FAILED;

        default:
            Print("WiFi Station", "WiFi start failed for unknown reason!");
            ClearData();
            return WIFI_STARTUP_FAILED;
    }

    return WIFI_STARTUP_OKAY;
}

int StartWiFiStation(
    void (*callback)(void), 
    void (*disconnection_handler)(void), 
    void (*stop_handler)(void), 
    void (*exhaustion_callback)(void),
    void (*bad_config_callback)(void)
) {
    wifi_callbacks.callback               = callback;
    wifi_callbacks.disconnection_handler  = disconnection_handler;
    wifi_callbacks.stop_handler           = stop_handler;
    wifi_callbacks.exhaustion_callback    = exhaustion_callback;
    wifi_callbacks.bad_config_callback    = bad_config_callback;
    
    switch(ConnectWithGlobalConfig())
    {
        case WIFI_STARTUP_OKAY:
            Print("WiFi Station", "WiFi started.");
            break;

        case WIFI_ALREADY_INSTANTIATED: return WIFI_ALREADY_INSTANTIATED;
        case WIFI_STARTUP_FAILED:       return WIFI_STARTUP_FAILED;
        case WIFI_STA_INIT_FAILED:      return WIFI_STA_INIT_FAILED;
        case WIFI_CONFIG_BAD:           return WIFI_CONFIG_BAD;
        case WIFI_CONFIG_NVS_ERR:       return WIFI_CONFIG_NVS_ERR;
        default:                        return WIFI_SETUP_SETUP_FAILED;
    }

    switch(StartWiFiTask())
    {
        case WIFI_TASK_CREATED:
            Print("WiFi Station", "WiFi task created.");
			break;

        case WIFI_TASK_ALREADY_CREATED: return WIFI_SETUP_INVALID;
        case WIFI_TASK_MEMORY_FAILURE:  return WIFI_SETUP_MEMORY_ERR;
        case WIFI_TASK_UNKNOWN_FAILURE: return WIFI_SETUP_SETUP_FAILED;
        default:                        return WIFI_SETUP_SETUP_FAILED;
    }

    switch(StartWiFiTerminationListener())
    {
        case WIFI_TASK_CREATED:
            Print("WiFi Station", "WiFi termination listener task created.");
			break;

        case WIFI_TASK_ALREADY_CREATED: return WIFI_SETUP_INVALID;
        case WIFI_TASK_MEMORY_FAILURE:  return WIFI_SETUP_MEMORY_ERR;
        case WIFI_TASK_UNKNOWN_FAILURE: return WIFI_SETUP_SETUP_FAILED;
        default:                        return WIFI_SETUP_SETUP_FAILED;
    }

    //TODO: double check ??? uh

    return WIFI_STARTUP_OKAY;
}


//BAD AUTH
void Reject(void)
{
    Print("WiFi Station", "Authentication failed. Setup configuration should be rejected - executing bad config callback.");
    Termination();
    wifi_callbacks.bad_config_callback();
}

//Can't find etc..
void Exhaustion(void)
{
    Print("WiFi Station", "Exhaustion. Setup configuration might be invalid - executing exhaustion callback.");
    Termination();
    wifi_callbacks.exhaustion_callback();
}

//Beacon time outs etc - so not invalid but failure nonetheless.
void InnocentExhaustion(void)
{
    Print("WiFi Station", "Innocent exhaustion. Setup configuration could be invalid - executing exhaustion callback.");
    Termination();
    wifi_callbacks.exhaustion_callback();
}

//Unhandled error so not really something we wanna try again just in case.
void UnknownExhaustion(void)
{
    Print("WiFi Station", "Unhandled exhaustion. Setup configuration might be invalid, not sure - executing bad config callback to be safe.");
    Termination();
    wifi_callbacks.bad_config_callback();
}


void Connected(void)
{
    Print("WiFi Station", "Connected to AP.");
    wifi_callbacks.callback();
}

void Disconnected(void)
{
    Print("WiFi Station", "Disconnected from AP.");
    wifi_callbacks.disconnection_handler();
}

void InvalidDuality(void)
{
    //Disconnect and reconnect to reset connection
    Print("WiFi Station", "Invalid duality, could indicate critical errors - terminating! If issue persists please check the src or open a github issue!");
    Termination();
}

//TODO: NEED TO DEREGISTER THE HANDLERS!!
int Terminate(void)
{
    StopWiFiTask();
    KillWiFiTerminationListener(); //Should kill that task gracefully through dissociated handler task even if said task called this.

    switch(esp_event_handler_unregister(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &_wifi_event_handler
    )) 
    {
        case ESP_OK:
            Print("WiFi Station", "WiFi event handler unregistered.");
            break;

        case ESP_ERR_INVALID_ARG:
            Print("WiFi Station", "Invalid combination of event base and event ID?! Please check src or open a github issue!");
            return WIFI_SHUTDOWN_UNSAVABLE;

        default:
            Print("WiFi Station", "WiFi event handler unregister failed for unknown reason!");
            return WIFI_SHUTDOWN_UNSAVABLE;
    }

    switch(esp_event_handler_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &_ip_event_handler
    )) 
    {
        case ESP_OK:
            Print("WiFi Station", "IP event handler unregistered.");
            break;

        case ESP_ERR_INVALID_ARG:
            Print("WiFi Station", "Invalid combination of event base and event ID?! Please check src or open a github issue!");
            return WIFI_SHUTDOWN_UNSAVABLE;

        default:
            Print("WiFi Station", "IP event handler unregister failed for unknown reason!");
            return WIFI_SHUTDOWN_UNSAVABLE;
    }

    if(wifi_event_group != NULL)
    {
        wifi_event_group = NULL;
    }

    if(sta_netif != NULL)
    {
        esp_netif_destroy(sta_netif);
        sta_netif = NULL;
    }

    switch(esp_wifi_stop())
    {
        case ESP_OK:
            Print("WiFi Station", "WiFi backend stopped.");
            break;

        case ESP_ERR_WIFI_NOT_INIT:
            Print("WiFi Station", "WiFi backend not running.");
            return WIFI_SHUTDOWN_NOT_INIT;

        default:
            Print("WiFi Station", "WiFi backend stop failed for unhandled reason!");
            return WIFI_SHUTDOWN_UNSAVABLE;
    }

    disconnection_reason_buffer             = 255;
    wifi_handles.wifi                       = NULL;
    wifi_handles.ip                         = NULL;

    /* These are needed but I want them gone...
    // Not sure if ensuring removal is useful in any way but I'd like too with a redesign of calling this?
    // - Maybe send this task a callback, and call it here to keep this as the final function in the chain?
    // - Could potentially store the callback in a stack variable before calling Termination()? Not sure...
    // Either way; the start method requires callback params and not specifying one would require a NULL assignment anyway.
    wifi_callbacks.callback                 = NULL;
    wifi_callbacks.disconnection_handler    = NULL;
    wifi_callbacks.stop_handler             = NULL;
    wifi_callbacks.exhaustion_callback      = NULL;
    wifi_callbacks.bad_config_callback      = NULL;
    */

    memset(&wifi_ip_data, 0, sizeof(wifi_ip_data));
    memset(&wifi_client_config, 0, sizeof(wifi_client_config));

    wifi_callbacks.stop_handler();

    return WIFI_SHUTDOWN_OKAY;
}

void Termination_Handler(void *pvParameters)
{
    Print("WiFi Station", "Termination handler task created.");
    switch(Terminate())
    {
        case WIFI_SHUTDOWN_OKAY:
            Print("WiFi Station", "WiFi shutdown okay.");
            break;

        case WIFI_SHUTDOWN_NOT_INIT:
            Print("WiFi Station", "WiFi shutdown failed - WiFi not initialized.");
            break;

        case WIFI_SHUTDOWN_UNSAVABLE:
            Print("WiFi Station", "WiFi shutdown failed for unhanlded reason!");
            break;

        default:
            Print("WiFi Station", "WiFi shutdown failed for unknown reason...");
            break;
    }
    vTaskDelete(NULL);
}

void Termination(void)
{
    Print("WiFi Station", "Termination request - creating termination handler task to dissociate from any impacted task callees.");
    xTaskCreate(
        Termination_Handler,
        "Termination-Handler",
        WIFI_TERMINATION_HANDLER_STACK,
        NULL,
        tskIDLE_PRIORITY,
        NULL
    );
}