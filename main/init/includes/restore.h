#include <commons.h>

#include <nvs.h>
#include <nvs_flash.h>
#include <esp_err.h>

#include <ap_config.h>

#define UPDATE_RESTORE_OK       0
#define UPDATE_RESTORE_NULL     1
#define UPDATE_RESTORE_NVSERR   2
#define UPDATE_RESTORE_UNKNOWN  3

#define ATTEMPT_RESTORE_OK      0
#define ATTEMPT_RESTORE_NONE    1
#define ATTEMPT_RESTORE_NULL    2
#define ATTEMPT_RESTORE_NVSERR  3
#define ATTEMPT_RESTORE_UNKNOWN  4

typedef struct RESTORE_CONFIG_STRUCTURE {
    wifi_connection_details_t wifi_details;
    mqtt_config_t mqtt_config;
    boot_config_t boot_config;
} restore_config_t;

int attempt_restore(void);
int update_restore_config(wifi_connection_details_t *wifiDetails, mqtt_config_t *mqttDetails, boot_config_t *bootDetails);