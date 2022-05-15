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
    void (*exhaustion_callback)(void)
) {
    wifi_callbacks.callback               = callback;
    wifi_callbacks.disconnection_handler  = disconnection_handler;
    wifi_callbacks.stop_handler           = stop_handler;
    wifi_callbacks.exhaustion_callback    = exhaustion_callback;
    
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


void Reject(void)
{
    //Callback faliure.
}

void Exhaustion(void)
{
    //Callback faliure.
}

void InnocentExhaustion(void)
{
    //Callback failure.
}

void UnknownExhaustion(void)
{
    //Callback failure?
}


void Connected(void)
{
    //Callback handling only
}

void Disconnected(void)
{
    //Callback handlers blah
}

void InvalidDuality(void)
{
    //Disconnect and reconnect to reset connection
}

void Termination(void)
{
    StopWiFiTask();
    //Callback handlers blah
    //Stop the background task and dissassemble the class (but using the actual callers etc.)
}