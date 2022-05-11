#include "restore.h"

static restore_config_t local_restore_data;

int attempt_restore(void)
{
    nvs_handle_t nvs;

    switch(nvs_open("storage", NVS_READWRITE, &nvs))
    {
        case ESP_OK:
            Print("Config Restoration", "NVS opened");
            break;

        case ESP_ERR_NVS_NOT_INITIALIZED:
            Print("Config Restoration", "NVS not initialized");
            Print("Config Restoration", "Unable to open config from NVS, aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ESP_ERR_NVS_PART_NOT_FOUND:
            Print("Config Restoration", "NVS partition not found");
            Print("Config Restoration", "Unable to open config from NVS, aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ESP_ERR_NVS_INVALID_NAME:
            Print("Config Restoration", "NVS invalid name");
            Print("Config Restoration", "Unable to open config from NVS, aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ESP_ERR_NO_MEM:
            Print("Config Restoration", "NVS no memory");
            Print("Config Restoration", "Unable to open config from NVS, aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        default:
            Print("Config Restoration", "NVS unknown error");
            Print("Config Restoration", "Unable to open config from NVS, aborting...");
            return ATTEMPT_RESTORE_NVSERR;
    }

    size_t size = sizeof(local_restore_data);
    switch(nvs_get_blob(nvs, "restore", &local_restore_data, &size))
    {
        case ESP_OK:
            Print("Config Restoration", "Restore data found");
            Print("Config Restoration", "Restoring config...");
            break;

        case ESP_ERR_NVS_NOT_FOUND:
            Print("Config Restoration", "Restore data not found");
            Print("Config Restoration", "No config to restore, letting caller know...");
            return ATTEMPT_RESTORE_NONE;

        case ESP_ERR_NVS_INVALID_HANDLE:
            Print("Config Restoration", "NVS invalid handle");
            Print("Config Restoration", "This error shouldn't occur, something has gone wrong in the NVS handler open function, please check for any descrepancies in the src.");
            Print("Config Restoration", "Unable to restore config from NVS, aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ESP_ERR_NVS_INVALID_NAME:
            Print("Config Restoration", "NVS invalid name");
            Print("Config Restoration", "This error shouldn't occur, something has gone wrong in the NVS handler open function, please check for any descrepancies in the src.");
            Print("Config Restoration", "Unable to restore config from NVS, aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ESP_ERR_NVS_INVALID_LENGTH:
            Print("Config Restoration", "NVS invalid length");
            Print("Config Restoration", "This error shouldn't occur, something has gone wrong in the NVS handler open function, please check for any descrepancies in the src.");
            Print("Config Restoration", "Attempting to clean up issue and act like no restoration data was found...");
            //TODO Actually clean up.
            return ATTEMPT_RESTORE_NONE;

        default:
            Print("Config Restoration", "NVS unknown error");
            Print("Config Restoration", "Unable to restore config from NVS, aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;
    }

    nvs_close(nvs);
    //TODO: Validate data recovered from NVS

    Print("Config Restoration", "Config restored, setting global init variables from recovered data...");

    set_wifi_details(local_restore_data.wifi_details.ssid, WIFI_SSID_BUFFER_MAX, local_restore_data.wifi_details.pass, WIFI_PASS_BUFFER_MAX);
    set_mqtt_config(local_restore_data.mqtt_config.address, MQTT_CONFIG_ADDRESS_LENGTH, local_restore_data.mqtt_config.username, MQTT_CONFIG_USERNAME_LENGTH);
    set_boot_config(local_restore_data.boot_config.mode, local_restore_data.boot_config.delay);

    Print("Config Restoration", "Restoration complete!");

    return ATTEMPT_RESTORE_OK;
}

int update_restore_config(wifi_connection_details_t *wifiDetails, mqtt_config_t *mqttDetails, boot_config_t *bootDetails)
{
    if(wifiDetails == NULL || mqttDetails == NULL || bootDetails == NULL)
    {
        Print("Config Restoration", "WiFi, MQTT or Boot details pointer is NULL");
        return UPDATE_RESTORE_NULL;
    }

    nvs_handle_t nvs;

    switch(nvs_open("storage", NVS_READWRITE, &nvs))
    {
        case ESP_OK:
            Print("Config Restoration", "NVS opened");
            break;

        case ESP_ERR_NVS_NOT_INITIALIZED:
            Print("Config Restoration", "NVS not initialized");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        case ESP_ERR_NVS_PART_NOT_FOUND:
            Print("Config Restoration", "NVS partition not found");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        case ESP_ERR_NVS_INVALID_NAME:
            Print("Config Restoration", "NVS invalid name");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        case ESP_ERR_NO_MEM:
            Print("Config Restoration", "NVS no memory");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        default:
            Print("Config Restoration", "NVS unknown error");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_UNKNOWN;
    }

    memset(&local_restore_data, '\0', sizeof(restore_config_t));
    local_restore_data.wifi_details = *wifiDetails;
    local_restore_data.mqtt_config = *mqttDetails;
    local_restore_data.boot_config = *bootDetails;

    switch(nvs_set_blob(nvs, "restore", &local_restore_data, sizeof(restore_config_t)))
    {
        case ESP_OK:
            Print("Config Restoration", "NVS restore value updated");
            break;

        case ESP_ERR_NVS_INVALID_HANDLE:
            Print("Config Restoration", "NVS invalid handle");
            Print("Config Restoration", "This error shouldn't occur, something has gone wrong in the NVS handler open function, please check for any descrepancies in the src.");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;
        
        case ESP_ERR_NVS_READ_ONLY:
            Print("Config Restoration", "NVS read only");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        case ESP_ERR_NVS_INVALID_NAME:
            Print("Config Restoration", "NVS invalid name");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
            Print("Config Restoration", "NVS not enough space");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        case ESP_ERR_NVS_VALUE_TOO_LONG:
            Print("Config Restoration", "NVS value too long");
            Print("Config Restoration", "This shouldn't occur, please check the src for any descrepancies.");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        default:
            Print("Config Restoration", "NVS unknown error");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_UNKNOWN;
    }

    switch(nvs_commit(nvs))
    {
        case ESP_OK:
            Print("Config Restoration", "NVS changes committed!");
            break;
        
        case ESP_ERR_NVS_INVALID_HANDLE:
            Print("Config Restoration", "NVS invalid handle");
            Print("Config Restoration", "This error shouldn't occur, something has gone wrong in the NVS handler open function, please check for any descrepancies in the src.");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_NVSERR;

        default:
            Print("Config Restoration", "NVS unknown error");
            Print("Config Restoration", "Unable to save config to NVS, aborting...");
            return UPDATE_RESTORE_UNKNOWN;
    }

    nvs_close(nvs);

    Print("Config Restoration", "Restore config updated.");

    return UPDATE_RESTORE_OK;
}
