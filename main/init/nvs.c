#include "nvs_init.h"

#include <nvs.h>
#include <nvs_flash.h>
#include <esp_err.h>

static int ReinitialiseNVS(void)
{
    esp_err_t erasure_result = nvs_flash_erase();
    switch(erasure_result)
    {
        case ESP_OK:
            Print("NVS", "NVS flash erased");
            break;

        case ESP_ERR_NVS_NO_FREE_PAGES: //Retry maybe?
        default:
            Print("NVS", "NVS flash erasure failed!");
            return NVS_INIT_PANIC;
    }

    esp_err_t init_result = nvs_flash_init();
    switch(init_result)
    {
        case ESP_OK:
            Print("NVS", "NVS flash initialised");
            return NVS_INIT_OK;

        case ESP_ERR_NVS_NO_FREE_PAGES:
        default:
            Print("NVS", "NVS flash initialisation failed!");
            return NVS_INIT_PANIC;
    }
}

int InitialiseNVS(void)
{
    Print("NVS", "Initialising NVS");
    esp_err_t nvs_result = nvs_flash_init();
    switch(nvs_result)
    {
        case ESP_OK:
            Print("NVS", "NVS flash initialised");
            return NVS_INIT_OK;

        case ESP_ERR_NVS_NEW_VERSION_FOUND:
            Print("NVS", "NVS flash version mismatch! Attempting to erase and reinitialise...");
            return ReinitialiseNVS();

        case ESP_ERR_NVS_NO_FREE_PAGES:
            Print("NVS", "NVS flash full! Please backup your data then erase and try again.");
            return NVS_INIT_FULL;

        default:
            Print("NVS", "NVS flash initialisation failed!");
            return NVS_INIT_PANIC;
    }
}