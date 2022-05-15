#include <commons.h>

#define WIFI_STARTUP_OKAY           0
#define WIFI_ALREADY_INSTANTIATED   1
#define WIFI_STARTUP_FAILED         2
#define WIFI_STA_INIT_FAILED        3
#define WIFI_CONFIG_BAD             4
#define WIFI_CONFIG_NVS_ERR         5

#define WIFI_STATION_STARTED        BIT0
#define WIFI_STATION_CONNECTED      BIT1
#define WIFI_STATION_DISCONNECTED   BIT2
#define WIFI_STATION_STOPPING       BIT3
#define WIFI_STATION_HAS_IP         BIT4
#define WIFI_STATION_LOST_IP        BIT5

int StartWiFiStation(void (*callback)(void), void (*disconnection_handler)(void), void (*stop_handler)(void), void (*exhaustion_callback)(void));