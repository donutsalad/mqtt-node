#include <commons.h>

#include <freertos/event_groups.h>

#define WIFI_TERMINATION_LISTENER_STACK     2048
#define WIFI_BACKGROUND_TASK_STACK          2048

#define WIFI_STARTUP_OKAY           0
#define WIFI_ALREADY_INSTANTIATED   1
#define WIFI_STARTUP_FAILED         2
#define WIFI_STA_INIT_FAILED        3
#define WIFI_CONFIG_BAD             4
#define WIFI_CONFIG_NVS_ERR         5

#define WIFI_SETUP_INVALID          6
#define WIFI_SETUP_MEMORY_ERR       7
#define WIFI_SETUP_SETUP_FAILED     8

#define WIFI_TASK_CREATED           0
#define WIFI_TASK_ALREADY_CREATED   1
#define WIFI_TASK_MEMORY_FAILURE    2
#define WIFI_TASK_UNKNOWN_FAILURE   3

#define WIFI_STATION_STARTED        BIT0
#define WIFI_STATION_CONNECTED      BIT1
#define WIFI_STATION_DISCONNECTED   BIT2
#define WIFI_STATION_STOPPING       BIT3
#define WIFI_STATION_HAS_IP         BIT4
#define WIFI_STATION_LOST_IP        BIT5

#define WIFI_DISCONNECTION_UNSPECIFIED              1
#define WIFI_DISCONNECTION_AUTH_EXPIRE              2
#define WIFI_DISCONNECTION_AUTH_LEAVE               3
#define WIFI_DISCONNECTION_ASSOC_EXPIRE             4
#define WIFI_DISCONNECTION_ASSOC_TOOMANY            5
#define WIFI_DISCONNECTION_NOT_AUTHED               6
#define WIFI_DISCONNECTION_NOT_ASSOCED              7
#define WIFI_DISCONNECTION_ASSOC_LEAVE              8
#define WIFI_DISCONNECTION_ASSOC_NOT_AUTHED         9
#define WIFI_DISCONNECTION_DISASSOC_PWRCAP_BAD      10
#define WIFI_DISCONNECTION_DISASSOC_SUPCHAN_BAD     11
#define WIFI_DISCONNECTION_IE_INVALID               13
#define WIFI_DISCONNECTION_MIC_FAILURE              14
#define WIFI_DISCONNECTION_4WAY_HANDSHAKE_TIMEOUT   15
#define WIFI_DISCONNECTION_GROUP_KEY_UPDATE_TIMEOUT 16
#define WIFI_DISCONNECTION_IE_IN_4WAY_DIFFERS       17
#define WIFI_DISCONNECTION_GROUP_CIPHER_INVALID     18
#define WIFI_DISCONNECTION_PAIRWISE_CIPHER_INVALID  19
#define WIFI_DISCONNECTION_AKMP_INVALID             20
#define WIFI_DISCONNECTION_UNSUPP_RSN_IE_VERSION    21
#define WIFI_DISCONNECTION_INVALID_RSN_IE_CAP       22
#define WIFI_DISCONNECTION_802_1X_AUTH_FAILED       23
#define WIFI_DISCONNECTION_CIPHER_SUITE_REJECTED    24

#define WIFI_DISCONNECTION_BEACON_TIMEOUT           200
#define WIFI_DISCONNECTION_NO_AP_FOUND              201
#define WIFI_DISCONNECTION_AUTH_FAIL                202
#define WIFI_DISCONNECTION_ASSOC_FAIL               203
#define WIFI_DISCONNECTION_HANDSHAKE_TIMEOUT        204

#define DISCONNECT_RESPONCE_RETRY       0
#define DISCONNECT_RESPONCE_REJECT      1
#define DISCONNECT_RESPONCE_RETRYWAIT   2
#define DISCONNECT_RESPONCE_NONEFOUND   3
#define DISCONNECT_RESPONCE_UNKNOWN     4

#define DISCONNECT_RESPONCE_RETRY_TIMEOUT_MS        2500
#define DISCONNECT_RESPONCE_NONEFOUND_TIMEOUT_MS    5000

#define RETRY_MAX_INNOCENT  3
#define RETRY_MAX_EXHAUSTED 5
#define RETRY_MAX_UNKNOWN   5

int StartWiFiStation(void (*callback)(void), void (*disconnection_handler)(void), void (*stop_handler)(void), void (*exhaustion_callback)(void));

int StartWiFiTask(void);
void StopWiFiTask(void);
int StartWiFiTerminationListener(void);

void Reject(void);
void Exhaustion(void);
void InnocentExhaustion(void);
void UnknownExhaustion(void);

void Connected(void);
void Disconnected(void);
void InvalidDuality(void);

void Termination(void);

EventGroupHandle_t get_wifi_event_group(void);
uint8_t get_disconnection_reason(void);

int DisconnectionEntailment(void);