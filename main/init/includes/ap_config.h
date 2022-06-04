#include <commons.h>
#include <htmlsrc.h>
#include <string.h>
#include <stdlib.h>

#include "datatypes.h"

#define AP_USER_CONNECTED       BIT0
#define AP_USER_DISCONNECTED    BIT1

#define AP_SSID                 "ESP32 WiFi Config"
#define AP_PASSWORD             "mqttnode"

#define AP_CHANNEL              1
#define AP_MAXCONNECTIONS       4
#define AP_BEACON_INTERVAL      100

#define AP_IERR_UNSAVABLE       0
#define AP_IERR_OK              1
#define AP_IERR_NVS_PROBLEM     2
#define AP_IERR_NO_MEM          3
#define AP_IERR_WIFI_NOINIT     4
#define AP_IERR_REBOOT          5
#define AP_IERR_BADPASSWORD     6

#define AP_ERR_UNSAVABLE        0
#define AP_ERR_OK               1
#define AP_ERR_NVS_PROBLEM      2
#define AP_ERR_REBOOT           3

#define WIFI_CONFIG_SSID_TAG    "s"
#define WIFI_CONFIG_PASS_TAG    "p"
//TODO: delete
#define WIFI_CONFIG_SET         "<html><head><title>ESP32 WiFi Config</title></head><body><h1>Configuration complete!</h1><p>Attempting to connect to the network, shutting down access point...</p></body></html>"

#define SERVER_CONFIG_OK        0
#define SERVER_CONFIG_BADFORMAT 1
#define SERVER_CONFIG_UNSAVABLE 2

#define SERVER_OK               0
#define SERVER_FATAL            1
#define SERVER_IGNORE           2
#define SERVER_EXISTS           3

#define SERVER_READY            BIT0
#define SERVER_WIFI_CONFIG      BIT1
#define SERVER_MQTT_CONFIG      BIT2
#define SERVER_BOOT_CONFIG      BIT3
#define SERVER_FINAL_SERVE      BIT4
#define SERVER_FCSS_SERVED      BIT5
#define SERVER_CONFIG_OKAY      BIT6 //Awful naming LOL, a typo taught me out
#define SERVER_SHUTDOWN_OK      BIT7
#define SERVER_STOP_FINISH      BIT8

#define MQTT_ENDPOINT_BUFFER    MQTT_CONFIG_ADDRESS_LENGTH
#define MQTT_USERNAME_BUFFER    MQTT_CONFIG_USERNAME_LENGTH

//I HATE THAT CSS IS ONLY THREE LETTERS LONG
//CRYING T-T); </3
#define WIFI_ENTITY_PAGE_CSS    0x0A
#define WIFI_ENTITY_WIFI_PAGE   0x10
#define WIFI_ENTITY_WIFI_DONE   0x11
#define WIFI_ENTITY_MQTT_PAGE   0x20
#define WIFI_ENTITY_MQTT_DONE   0x21
#define WIFI_ENTITY_DENY_BOOT   0xF0
#define WIFI_ENTITY_MODE_PAGE   0x30
#define WIFI_ENTITY_E400_PAGE   0x40
#define WIFI_ENTITY_E404_PAGE   0x50
#define WIFI_ENTITY_E500_PAGE   0x60
#define WIFI_ENTITY_DONE_PAGE   0x70

#define WIFI_SET_CODE_WIFI      0x01
#define WIFI_SET_CODE_MQTT      0x02
#define WIFI_SET_CODE_DENY      0x03
#define WIFI_SET_CODE_MODE      0x04
#define WIFI_SET_CODE_SAVE      0x05

#define RENDER_PAGE_ID_BOOT     0x00
#define RENDER_PAGE_ID_SAVE     0x01

typedef char wifi_entity_t;
typedef char wifi_set_code_t;
typedef char render_page_id_t;

int InitialiseAccessPoint(void);

void SetupWebserverEventListeners(void);
int InitialiseWebserver(void);
int TriggerWebserverClose(void);

void wait_for_config_okay(void);
void wait_for_webserver_start(void);
void wait_for_shutdown_completion(void);