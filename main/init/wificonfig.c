#include "ap_config.h"

#include <string.h>

static wifi_connection_details_t lan_details;

int set_wifi_details(char ssid[], int ssidBufferLength, char pass[], int passBufferLength)
{
    if(ssidBufferLength > WIFI_SSID_BUFFER_MAX || passBufferLength > WIFI_PASS_BUFFER_MAX)
    {
        Print("Init Manager", "WiFi details too long. Please check the src for any descrepencies.");
        return WIFI_DETAILS_TOBIG;
    }

    memset(lan_details.ssid, '\0', WIFI_SSID_BUFFER_MAX);
    memset(lan_details.pass, '\0', WIFI_PASS_BUFFER_MAX);

    strncpy(lan_details.ssid, ssid, ssidBufferLength);
    strncpy(lan_details.pass, pass, passBufferLength);

    Print("Init Manager", "WiFi details set.");
    return WIFI_DETAILS_OK;
}

int get_wifi_details(char ssid[], int ssidBufferLength, char pass[], int passBufferLength)
{
    if(ssidBufferLength > WIFI_SSID_BUFFER_MAX || passBufferLength > WIFI_PASS_BUFFER_MAX)
    {
        Print("Init Manager", "Recieving WiFi detail buffers too short. Please check the src for any descrepencies.");
        return WIFI_DETAILS_TOBIG;
    }

    memset(ssid, '\0', ssidBufferLength);
    memset(pass, '\0', passBufferLength);

    strncpy(ssid, lan_details.ssid, ssidBufferLength);
    strncpy(pass, lan_details.pass, passBufferLength);

    Print("Init Manager", "WiFi details retrieved.");
    return WIFI_DETAILS_OK;
}


int copy_wifi_details(wifi_connection_details_t *wifiDetails)
{
    if(wifiDetails == NULL)
    {
        Print("Init Manager", "WiFi details pointer is NULL.");
        return WIFI_DETAILS_NULL;
    }

    memset(wifiDetails->ssid, '\0', WIFI_SSID_BUFFER_MAX);
    memset(wifiDetails->pass, '\0', WIFI_PASS_BUFFER_MAX);

    strncpy(wifiDetails->ssid, lan_details.ssid, WIFI_SSID_BUFFER_MAX);
    strncpy(wifiDetails->pass, lan_details.pass, WIFI_PASS_BUFFER_MAX);

    Print("Init Manager", "WiFi details copied.");
    return WIFI_DETAILS_OK;
}

wifi_connection_details_t* wifi_details(void)
{
    return &lan_details;
}

int set_wifi_details_ssid(char ssid[], int ssidBufferLength)
{
    if(ssidBufferLength > WIFI_SSID_BUFFER_MAX)
    {
        Print("Init Manager", "WiFi details too long. Please check the src for any descrepencies.");
        return WIFI_DETAILS_TOBIG;
    }

    memset(lan_details.ssid, '\0', WIFI_SSID_BUFFER_MAX);
    strncpy(lan_details.ssid, ssid, ssidBufferLength);

    Print("Init Manager", "WiFi SSID set.");
    return WIFI_DETAILS_OK;
}

int set_wifi_details_pass(char pass[], int passBufferLength)
{
    if(passBufferLength > WIFI_PASS_BUFFER_MAX)
    {
        Print("Init Manager", "WiFi details too long. Please check the src for any descrepencies.");
        return WIFI_DETAILS_TOBIG;
    }

    memset(lan_details.pass, '\0', WIFI_PASS_BUFFER_MAX);
    strncpy(lan_details.pass, pass, passBufferLength);

    Print("Init Manager", "WiFi password set.");
    return WIFI_DETAILS_OK;
}

char* get_wifi_ssid_nullterm_safe(void)
{
    char *copy = calloc(WIFI_SSID_BUFFER_MAX + 1, sizeof(char));
    memset(copy, '\0', WIFI_SSID_BUFFER_MAX + 1);
    strncpy(copy, lan_details.ssid, WIFI_SSID_BUFFER_MAX);
    return copy;
}


char* get_wifi_pass_nullterm_safe(void)
{
    char *copy = calloc(WIFI_SSID_BUFFER_MAX + 1, sizeof(char));
    memset(copy, '\0', WIFI_PASS_BUFFER_MAX + 1);
    strncpy(copy, lan_details.pass, WIFI_PASS_BUFFER_MAX);
    return copy;
}
