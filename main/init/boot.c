#include "ap_config.h"

static boot_config_t boot_details;

const char* bootconfig_type_full = BOOT_DETAILS_NAME_FULL;
const char* bootconfig_type_state = BOOT_DETAILS_NAME_STATE;
const char* bootconfig_type_log = BOOT_DETAILS_NAME_LOG;
const char* bootconfig_type_none = BOOT_DETAILS_NAME_NONE;
const char* bootconfig_type_err = "Unknown!? Check src...";

const char* bootconfig_type_from_int(int i)
{
    switch (i) {
        case 0: return bootconfig_type_full;
        case 1: return bootconfig_type_state;
        case 2: return bootconfig_type_log;
        case 3: return bootconfig_type_none;
        default: return bootconfig_type_err;
    }
}

int set_boot_config(int mode, int delay)
{
    if (delay < 0)
    {
        Print("Init Manager", "Boot delay cannot be negative");
        return BOOT_DETAILS_BADFORMAT;
    }

    boot_details.mode = mode;
    boot_details.delay = delay;

    Print("Init Manager", "Boot config set");
    return BOOT_DETAILS_OK;
}

int get_boot_config(int *mode, int *delay)
{
    if (mode == NULL || delay == NULL)
    {
        Print("Init Manager", "Boot config pointers are NULL");
        return BOOT_DETAILS_NULL;
    }

    *mode = boot_details.mode;
    *delay = boot_details.delay;

    Print("Init Manager", "Boot config retrieved");
    return BOOT_DETAILS_OK;
}

int copy_boot_config(boot_config_t *bootConfig)
{
    if (bootConfig == NULL)
    {
        Print("Init Manager", "Boot config pointer is NULL");
        return BOOT_DETAILS_NULL;
    }

    bootConfig->mode = boot_details.mode;
    bootConfig->delay = boot_details.delay;

    Print("Init Manager", "Boot config copied");
    return BOOT_DETAILS_OK;
}

boot_config_t* boot_config(void)
{
    return &boot_details;
}

void set_boot_config_mode(int mode)
{
    boot_details.mode = mode;
}

void set_boot_config_delay(int delay)
{
    boot_details.delay = delay;
}