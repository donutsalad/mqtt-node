#include "restore.h"

#include <datatypes.h>

static int restoration_code;
static restore_config_t local_restore_data;

//TODO: Validate for lack of bugs - I was super tired when I wrote this oml...
//The bugs could be hidden in pretty edge cases - if you find one well down the track feel free to open an issue.

static int _transfer_nvs_to_local_restore_data(void) {
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
    return ATTEMPT_RESTORE_OK;
}

static int _transfer_local_restore_data_nvs_opener(nvs_handle_t *nvs) {
    switch(nvs_open("storage", NVS_READWRITE, nvs))
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

    return UPDATE_RESTORE_OK;
}

static int _transfer_local_restore_data_to_nvs(nvs_handle_t nvs) {
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

    return UPDATE_RESTORE_OK;
}

//If you call this function please ensure the NVS and local restore data are in sync.
static int _validate_and_update_stored_restore_data(void) {
    Print("Config Restoration", "Validation co-routine entered.");

    /* Don't need to load data - since the caller functions should've loaded it to check already.
    switch(_transfer_nvs_to_local_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config restored locally; continuing...");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to restore; aborting...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst doing local restore; aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting local restore; aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }
    */

    nvs_handle_t nvs;
    switch(_transfer_local_restore_data_nvs_opener(&nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "NVS open successful, continuing...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to open NVS to validate config! Aborting...");
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while opening NVS to validate config! Aborting...");
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            return UPDATE_RESTORE_UNKNOWN;
    }

    //Keep all of the flags without wifi validation entailments and set the validation flag whilst clearing any other validation flags.
    local_restore_data.flags = (local_restore_data.flags & ~BOOT_FLAGS_V_MASK) | (BOOT_FLAG_VALIDATED);
    
    switch(_transfer_local_restore_data_to_nvs(nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "Config updated successfully, closing NVS handle...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to save config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while saving config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;
    }

    nvs_close(nvs);

    return UPDATE_RESTORE_OK;
}

//If you call this function please ensure the NVS and local restore data are in sync.
static int _fail_and_update_stored_restore_data(void) {
    Print("Config Restoration", "Failure co-routine entered.");

    /* Don't need to load data - since the caller functions should've loaded it to check already.
    switch(_transfer_nvs_to_local_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config restored locally; continuing...");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to restore; aborting...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst doing local restore; aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting local restore; aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }
    */

    nvs_handle_t nvs;
    switch(_transfer_local_restore_data_nvs_opener(&nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "NVS open successful, continuing...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to open NVS to fail config! Aborting...");
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while opening NVS to fail config! Aborting...");
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            return UPDATE_RESTORE_UNKNOWN;
    }

    //Just set the failed flag - it does't really mean anything else other then a failed attempt.
    local_restore_data.flags |= BOOT_FLAG_FAILED;
    
    switch(_transfer_local_restore_data_to_nvs(nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "Config updated successfully, closing NVS handle...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to save config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while saving config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;
    }

    nvs_close(nvs);

    return UPDATE_RESTORE_OK;
}

//If you call this function please ensure the NVS and local restore data are in sync.
static int _reject_auth_and_update_stored_restore_data(void) {
    Print("Config Restoration", "Bad Auth co-routine entered.");

    /* Don't need to load data - since the caller functions should've loaded it to check already.
    switch(_transfer_nvs_to_local_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config restored locally; continuing...");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to restore; aborting...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst doing local restore; aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting local restore; aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }
    */

    nvs_handle_t nvs;
    switch(_transfer_local_restore_data_nvs_opener(&nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "NVS open successful, continuing...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to open NVS to reject auth from config! Aborting...");
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while opening NVS to reject auth from config! Aborting...");
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            return UPDATE_RESTORE_UNKNOWN;
    }

    //Set both the failed flag and the bad auth flag - since it's implicit we failed if we have bad auth.
    local_restore_data.flags |= (BOOT_FLAG_FAILED | BOOT_FLAG_BAD_AUTH);
    
    switch(_transfer_local_restore_data_to_nvs(nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "Config updated successfully, closing NVS handle...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to save config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while saving config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;
    }

    nvs_close(nvs);

    return UPDATE_RESTORE_OK;
}

int get_boot_config_status(void)
{
    if(_boot_details_validated(&local_restore_data))
    {
        if(_boot_details_bad_auth(&local_restore_data))     return BOOT_STATUS_CHANGED;
        else if(_boot_details_failed(&local_restore_data))  return BOOT_STATUS_FALIDATED;
        else                                                return BOOT_STATUS_VALIDATED;
    }
    else if(_boot_details_bad_auth(&local_restore_data))    return BOOT_STATUS_BAD_WIFI;
    else if(_boot_details_failed(&local_restore_data))      return BOOT_STATUS_FAILED;
    else                                                    return BOOT_STATUS_UNKNOWN;
}

int attempt_restore(void)
{
    Print("Config Restoration", "Attempting to restore config...");

    restoration_code = RESTORATION_LACKING;

    switch(_transfer_nvs_to_local_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config restored locally; continuing...");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to restore; aborting...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst doing local restore; aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting local restore; aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }
    
    restoration_code = RESTORATION_USING;
    Print("Config Restoration", "Restoration successful, checking flags to see if the configuration is valid");

    switch(get_boot_config_status())
    {
        case BOOT_STATUS_UNKNOWN:
            Print("Config Restoration", "Configuration is yet to be confirmed, continuing.");
            break;

        case BOOT_STATUS_VALIDATED:
            Print("Config Restoration", "Configuration is valid, continuing...");
            break;

        case BOOT_STATUS_CHANGED:
            Print("Config Restoration", "Configuration previously validated - but now WiFi auth has changed. Letting the caller know.");
            return ATTEMPT_RESTORE_NEW_WIFI;

        case BOOT_STATUS_BAD_WIFI:
            Print("Config Restoration", "Configuration is invalid, WiFi details resulted in bad authentication attempt, Letting the caller know.");
            return ATTEMPT_RESTORE_BAD_WIFI;

        case BOOT_STATUS_FAILED:
            Print("Config Restoration", "Configuration is yet to successfully work, could just be out of range - continuing for at least one retry.");
            break;

        default:
            Print("Config Restoration", "Unhandled status code, aborting...");
            restoration_code = RESTORATION_LACKING;
            return ATTEMPT_RESTORE_UNKNOWN;
    }

    Print("Config Restoration", "Config is in a usable state, setting global init variables from recovered data...");

    set_wifi_details(local_restore_data.wifi_details.ssid, WIFI_SSID_BUFFER_MAX, local_restore_data.wifi_details.pass, WIFI_PASS_BUFFER_MAX);
    set_mqtt_config(local_restore_data.mqtt_config.address, MQTT_CONFIG_ADDRESS_LENGTH, local_restore_data.mqtt_config.username, MQTT_CONFIG_USERNAME_LENGTH);
    set_boot_config(local_restore_data.boot_config.mode, local_restore_data.boot_config.delay);

    Print("Config Restoration", "Restoration complete!");

    return ATTEMPT_RESTORE_OK;
}

int validate_restore_data(void)
{
    //Sync NVS
    switch(_transfer_nvs_to_local_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config restored locally; continuing...");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to restore; aborting...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst doing local restore; aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting local restore; aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }

    int buffer = -1;

    //Figure out if we have to do something, debug log what case, store the result or return early.
    switch(get_boot_config_status())
    {
        case BOOT_STATUS_VALIDATED:
            Print("Config Restoration", "Configuration is already set as validated, letting caller know nothings changed.");
            return FLAGS_SET_UNCHANGED;

        case BOOT_STATUS_FALIDATED:
            Print("Config Restoration", "Configuration was already validated, but had failed since - removing fail flag and letting caller know we changed something else too.");
            buffer = FLAGS_SET_TWEAKED;
            break;

        case BOOT_STATUS_FAILED:
            Print("Congif Restoration", "Configuration had failed without validation, but is now valid - removing fail flag and letting caller know we changed something else too.");
            buffer = FLAGS_SET_TWEAKED;
            break;

        case BOOT_STATUS_CHANGED:
        case BOOT_STATUS_BAD_WIFI:
            Print("Config Restoration", "Configuration thought that WiFi details were bad...? Clearing the malasumptions and letting caller know things changed.");
            Print("Config Restoration", "If there are other bugs abound and you're reading this message - check any WiFi detail codes or mention this in the github issue.");
            buffer = FLAGS_SET_TWEAKED;
            break;

        case BOOT_STATUS_UNKNOWN:
            Print("Config Restoration", "Configuration was fresh - setting validation flag");
            buffer = FLAGS_SET_SINGLE;
            break;

        default: 
            Print("Config Restoration", "Unknown current status... aborting in case we break something.");
            return FLAGS_SET_UNHANDLED;
    }
    
    switch(_validate_and_update_stored_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config validated.");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to validate?!?! Not sure how we got here, please check src and/or open a github issue...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst validating config! Aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting to validate config. Aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }

    return buffer;
}

int failed_with_config(void)
{
    //Sync NVS
    switch(_transfer_nvs_to_local_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config restored locally; continuing...");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to restore; aborting...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst doing local restore; aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting local restore; aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN; //Bad
    }

    int buffer = -1;
    
    //Why are we theoretically getting to this point and stopping...?

    //Figure out if we have to do something, debug log what case, store the result or return early.
    switch(get_boot_config_status())
    {
        case BOOT_STATUS_FAILED:
        case BOOT_STATUS_FALIDATED:
            Print("Config Restoration", "Configuration is already set as failed, letting caller know nothings changed.");
            return FLAGS_SET_UNCHANGED;

        case BOOT_STATUS_BAD_WIFI:
        case BOOT_STATUS_CHANGED:
            Print("Config Restoration", "Configuration has bad WiFi details, Failure is assumed.");
            return FLAGS_SET_UNCHANGED;

        case BOOT_STATUS_UNKNOWN:
            Print("Config Restoration", "Configuration was fresh - setting faliure flag");
            buffer = FLAGS_SET_SINGLE;
            break;

        case BOOT_STATUS_VALIDATED:
            Print("Config Restoration", "Configuration was validated! Could be out of range - setting failed flag.");
            buffer = FLAGS_SET_SINGLE;
            break;

        default: 
            Print("Config Restoration", "Unknown current status... aborting in case we break something.");
            return FLAGS_SET_UNHANDLED;
    }
    
    switch(_fail_and_update_stored_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config validated.");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to fail?!?! Not sure how we got here, please check src and/or open a github issue...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst failing config! Aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting to fail config. Aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }

    return buffer;
}

int bad_auth_from_config(void)
{
    //Sync NVS
    switch(_transfer_nvs_to_local_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config restored locally; continuing...");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to restore; aborting...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst doing local restore; aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting local restore; aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }

    int buffer = -1;

    //Figure out if we have to do something, debug log what case, store the result or return early.
    switch(get_boot_config_status())
    {
        case BOOT_STATUS_BAD_WIFI:
        case BOOT_STATUS_CHANGED:
            Print("Config Restoration", "Configuration has bad WiFi details already, letting caller know nothings changed.");
            return FLAGS_SET_UNCHANGED;

        case BOOT_STATUS_FAILED:
        case BOOT_STATUS_FALIDATED:
            Print("Config Restoration", "Configuration is already set as failed, only the bad auth flag will be changed.");
            return FLAGS_SET_SINGLE;

        case BOOT_STATUS_UNKNOWN:
            Print("Config Restoration", "Configuration was fresh - setting both faliure and bad auth flags.");
            buffer = FLAGS_SET_TWEAKED;
            break;

        case BOOT_STATUS_VALIDATED:
            Print("Config Restoration", "Configuration was validated! The details have likely changed - validation flag will remain and both failure and bad auth flags will be set.");
            buffer = FLAGS_SET_TWEAKED;
            break;

        default: 
            Print("Config Restoration", "Unknown current status... aborting in case we break something.");
            return FLAGS_SET_UNHANDLED;
    }
    
    switch(_reject_auth_and_update_stored_restore_data())
    {
        case ATTEMPT_RESTORE_OK:
            Print("Config Restoration", "Config invalidated.");
            break;

        case ATTEMPT_RESTORE_NONE:
            Print("Config Restoration", "No config to reject auth thereof?!?! Not sure how we got here, please check src and/or open a github issue...");
            return ATTEMPT_RESTORE_NONE;

        case ATTEMPT_RESTORE_NVSERR:
            Print("Config Restoration", "NVS error whilst rejecting auth from config! Aborting...");
            return ATTEMPT_RESTORE_NVSERR;

        case ATTEMPT_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error whilst attempting to reject auth from config. Aborting...");
            return ATTEMPT_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error from within the same src file!!! Please check src and/or open a github issue!");
            return ATTEMPT_RESTORE_UNKNOWN;
    }

    return buffer;
}

int using_restoration_data(void)
{
    switch(restoration_code)
    {
        case RESTORATION_LACKING:
            return RESTORATION_LACKING;

        case RESTORATION_USING:
            return RESTORATION_USING;

        default: return RESTORATION_UNDEFINED;
    }
}
//Sets all the flags to 0 - whatever individual state we could logically retain will automatically be arrived at anyway.
int update_restore_config(wifi_connection_details_t *wifiDetails, mqtt_config_t *mqttDetails, boot_config_t *bootDetails)
{
    if(wifiDetails == NULL || mqttDetails == NULL || bootDetails == NULL)
    {
        Print("Config Restoration", "WiFi, MQTT or Boot details pointer is NULL");
        return UPDATE_RESTORE_NULL;
    }

    Print("Config Restoration", "Updating config...");

    nvs_handle_t nvs;

    switch(_transfer_local_restore_data_nvs_opener(&nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "NVS open successful, continuing...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to open NVS to save config! Aborting...");
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while opening NVS to save config! Aborting...");
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            return UPDATE_RESTORE_UNKNOWN;
    }

    //This is what sets all flags to 0 - if this is removed then flags will need to be manually 0 set.
    memset(&local_restore_data, '\0', sizeof(restore_config_t));
    local_restore_data.wifi_details = *wifiDetails;
    local_restore_data.mqtt_config = *mqttDetails;
    local_restore_data.boot_config = *bootDetails;

    switch(_transfer_local_restore_data_to_nvs(nvs))
    {
        case UPDATE_RESTORE_OK:
            Print("Config Restoration", "Config updated successfully, closing NVS handle...");
            break;

        case UPDATE_RESTORE_NVSERR:
            Print("Config Restoration", "Unable to save config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_NVSERR;

        case UPDATE_RESTORE_UNKNOWN:
            Print("Config Restoration", "Unknown error occured while saving config to NVS! Aborting...");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;

        default:
            Print("Config Restoration", "Unknown error occured from within the same file!!! Please check src and/or open a github issue!");
            nvs_close(nvs);
            return UPDATE_RESTORE_UNKNOWN;
    }

    nvs_close(nvs);

    Print("Config Restoration", "Restore config updated.");

    restoration_code = RESTORATION_USING;
    return UPDATE_RESTORE_OK;
}
