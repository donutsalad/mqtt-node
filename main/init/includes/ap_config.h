#include <commons.h>

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
//TODO: concat the above in the below
#define WIFI_CONFIG_PAGE        "<html><head><title>ESP32 WiFi Config</title></head><body><h1>Configure LAN Access Point Details</h1><form action=\"/set\" method=\"POST\"><div><label for=\"s\">Network SSID (Name)</label><input name=\"s\" id=\"s\" placeholder=\"Your Network Here\"></div><div><label for=\"p\">Network Password</label><input name=\"p\" id=\"p\" placeholder=\"Password\"></div><div><button>Connect</button></div></form></body></html>"
#define WIFI_CONFIG_SET         "<html><head><title>ESP32 WiFi Config</title></head><body><h1>Configuration complete!</h1><p>Attempting to connect to the network, shutting down access point...</p></body></html>"

#define WIFI_CONFIG_OK          0
#define WIFI_CONFIG_BADFORMAT   1
#define WIFI_CONFIG_UNSAVABLE   2

#define SERVER_OK               0
#define SERVER_FATAL            1
#define SERVER_IGNORE           2
#define SERVER_EXISTS           3

#define SERVER_READY            BIT0
#define SERVER_CONFIG_OK        BIT1
#define SERVER_SET_CONFIG_DONE  BIT2
#define SERVER_SHUTDOWN         BIT3

#define WIFI_SSID_BUFFER_MAX    64
#define WIFI_PASS_BUFFER_MAX    64

#define WIFI_DETAILS_OK         0
#define WIFI_DETAILS_TOBIG      1

int InitialiseAccessPoint(void);

void SetupWebserverEventListeners(void);
int InitialiseWebserver(void);
int TriggerWebserverClose(void);

void wait_for_wifi_config(void);
void wait_for_webserver_start(void);
void wait_for_shutdown_completion(void);

typedef struct WIFI_DETAILS_STRUCTURE {
    char ssid[WIFI_SSID_BUFFER_MAX];
    char pass[WIFI_PASS_BUFFER_MAX];
} wifi_connection_details_t;

int set_wifi_details(char ssid[], int ssidBufferLength, char pass[], int passBufferLength);
int get_wifi_details(char ssid[], int ssidBufferLength, char pass[], int passBufferLength);

int copy_wifi_details(wifi_connection_details_t *wifiDetails);
wifi_connection_details_t* wifi_details(void);

int set_wifi_details_ssid(char ssid[], int ssidBufferLength);
int set_wifi_details_pass(char pass[], int passBufferLength);

char* get_wifi_ssid_nullterm_safe(void);
char* get_wifi_pass_nullterm_safe(void);
