#include <commons.h>
#include <htmlsrc.h>
#include <string.h>
#include <stdlib.h>

#define WIFI_SSID_BUFFER_MAX    64
#define WIFI_PASS_BUFFER_MAX    64

#define MQTT_CONFIG_ADDRESS_LENGTH  64
#define MQTT_CONFIG_USERNAME_LENGTH 64

#define WIFI_DETAILS_OK         0
#define WIFI_DETAILS_TOBIG      1
#define WIFI_DETAILS_NULL       2

#define MQTT_DETAILS_OK         0
#define MQTT_DETAILS_TOBIG      1
#define MQTT_DETAILS_NULL       2

#define BOOT_MODE_BUFFER        4
#define BOOT_DELAY_BUFFER       16

#define BOOT_DETAILS_OK         0
#define BOOT_DETAILS_TOBIG      1
#define BOOT_DETAILS_NULL       2
#define BOOT_DETAILS_BADFORMAT  3

#define BOOT_DETAILS_TAG_FULL   "f"
#define BOOT_DETAILS_TAG_STATE  "c"
#define BOOT_DETAILS_TAG_LOG    "l"
#define BOOT_DETAILS_TAG_NONE   "n"

#define BOOT_DETAILS_NAME_FULL  "Full State Poll"
#define BOOT_DETAILS_NAME_STATE "Changes States Poll"
#define BOOT_DETAILS_NAME_LOG   "Logging Output Only"
#define BOOT_DETAILS_NAME_NONE  "No Poll"

#define UPDATE_RESTORE_OK       0
#define UPDATE_RESTORE_NULL     1
#define UPDATE_RESTORE_NVSERR   2
#define UPDATE_RESTORE_UNKNOWN  3

#define BOOT_FLAG_VALIDATED     0x01
#define BOOT_FLAG_FAILED        0x02
#define BOOT_FLAG_BAD_AUTH      0x04

#define BOOT_FLAGS_V_MASK       0b00000111

#define BOOT_STATUS_UNKNOWN     0
#define BOOT_STATUS_VALIDATED   1
#define BOOT_STATUS_FALIDATED   2
#define BOOT_STATUS_CHANGED     3
#define BOOT_STATUS_FAILED      4
#define BOOT_STATUS_BAD_WIFI    5

typedef struct WIFI_DETAILS_STRUCTURE {
    char ssid[WIFI_SSID_BUFFER_MAX];
    char pass[WIFI_PASS_BUFFER_MAX];
} wifi_connection_details_t;

typedef struct MQTT_CONFIG_STRUCT {
    char address[MQTT_CONFIG_ADDRESS_LENGTH];
    char username[MQTT_CONFIG_USERNAME_LENGTH];
} mqtt_config_t;

typedef struct BOOT_CONFIG_STRUCT {
    int mode;
    int delay;
} boot_config_t;

typedef struct RESTORE_CONFIG_STRUCTURE {
    uint8_t flags;
    wifi_connection_details_t wifi_details;
    mqtt_config_t mqtt_config;
    boot_config_t boot_config;
} restore_config_t;

inline int _boot_details_validated(restore_config_t *config) {
    return (config->flags & BOOT_FLAG_VALIDATED);
}

inline int _boot_details_failed(restore_config_t *config) {
    return (config->flags & BOOT_FLAG_FAILED);
}

inline int _boot_details_bad_auth(restore_config_t *config) {
    return (config->flags & BOOT_FLAG_BAD_AUTH);
}

inline int boot_config_mode_from_string(char *str) {
    if (strcmp(str, BOOT_DETAILS_TAG_FULL) == 0) {
        return 0;
    } else if (strcmp(str, BOOT_DETAILS_TAG_STATE) == 0) {
        return 1;
    } else if (strcmp(str, BOOT_DETAILS_TAG_LOG) == 0) {
        return 2;
    } else if (strcmp(str, BOOT_DETAILS_TAG_NONE) == 0) {
      return 3;  
    } else {
        return -1;
    }
}

const char* bootconfig_type_from_int(int i);

inline int boot_config_delay_from_string(char *str) {
    int delay = atoi(str);
    return delay;
}

int set_wifi_details(char ssid[], int ssidBufferLength, char pass[], int passBufferLength);
int get_wifi_details(char ssid[], int ssidBufferLength, char pass[], int passBufferLength);

int copy_wifi_details(wifi_connection_details_t *wifiDetails);
wifi_connection_details_t* wifi_details(void);

int set_wifi_details_ssid(char ssid[], int ssidBufferLength);
int set_wifi_details_pass(char pass[], int passBufferLength);

char* get_wifi_ssid_nullterm_safe(void);
char* get_wifi_pass_nullterm_safe(void);

int set_mqtt_config(char address[], int addressBufferLength, char username[], int usernameBufferLength);
int get_mqtt_config(char address[], int addressBufferLength, char username[], int usernameBufferLength);

int copy_mqtt_config(mqtt_config_t *mqttConfig);
mqtt_config_t* mqtt_config(void);

int set_mqtt_config_address(char address[], int addressBufferLength);
int set_mqtt_config_username(char username[], int usernameBufferLength);

char* get_mqtt_config_address_nullterm_safe(void);
char* get_mqtt_config_username_nullterm_safe(void);

int set_boot_config(int mode, int delay);
int get_boot_config(int *mode, int *delay);

int copy_boot_config(boot_config_t *bootConfig);
boot_config_t* boot_config(void);

void set_boot_config_mode(int mode);
void set_boot_config_delay(int delay);

int update_restore_config(wifi_connection_details_t *wifiDetails, mqtt_config_t *mqttDetails, boot_config_t *bootDetails);
int get_boot_config_status(void);