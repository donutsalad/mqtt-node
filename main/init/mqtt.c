#include "ap_config.h"

#include <string.h>

static mqtt_config_t mqtt_details;

int set_mqtt_config(char address[], int addressBufferLength, char username[], int usernameBufferLength)
{
    if (addressBufferLength > MQTT_CONFIG_ADDRESS_LENGTH || usernameBufferLength > MQTT_CONFIG_USERNAME_LENGTH) 
    {
        Print("Init Manager", "MQTT address or username too long. Please check the src for any descrepancies.");
        return MQTT_DETAILS_TOBIG;
    }

    memset(mqtt_details.address, 0, MQTT_CONFIG_ADDRESS_LENGTH);
    memset(mqtt_details.username, 0, MQTT_CONFIG_USERNAME_LENGTH);

    strncpy(mqtt_details.address, address, addressBufferLength);
    strncpy(mqtt_details.username, username, usernameBufferLength);

    Print("Init Manager", "MQTT details set");
    return MQTT_DETAILS_OK;
}

int get_mqtt_config(char address[], int addressBufferLength, char username[], int usernameBufferLength)
{
    if (addressBufferLength > MQTT_CONFIG_ADDRESS_LENGTH || usernameBufferLength > MQTT_CONFIG_USERNAME_LENGTH) 
    {
        Print("Init Manager", "Receiving MQTT detail buffers are too short. Please check the src for any descrepancies.");
        return MQTT_DETAILS_TOBIG;
    }

    memset(address, 0, addressBufferLength);
    memset(username, 0, usernameBufferLength);

    strncpy(address, mqtt_details.address, addressBufferLength);
    strncpy(username, mqtt_details.username, usernameBufferLength);

    Print("Init Manager", "MQTT details retrieved");
    return MQTT_DETAILS_OK;
}

int copy_mqtt_config(mqtt_config_t *mqttDetails)
{
    if (mqttDetails == NULL) 
    {
        Print("Init Manager", "MQTT details pointer is NULL");
        return WIFI_DETAILS_NULL;
    }

    memset(mqttDetails->address, 0, MQTT_CONFIG_ADDRESS_LENGTH);
    memset(mqttDetails->username, 0, MQTT_CONFIG_USERNAME_LENGTH);

    strncpy(mqttDetails->address, mqtt_details.address, MQTT_CONFIG_ADDRESS_LENGTH);
    strncpy(mqttDetails->username, mqtt_details.username, MQTT_CONFIG_USERNAME_LENGTH);

    Print("Init Manager", "MQTT details copied");
    return MQTT_DETAILS_OK;
}

mqtt_config_t *mqtt_config(void)
{
    return &mqtt_details;
}

int set_mqtt_config_address(char address[], int addressBufferLength)
{
    if (addressBufferLength > MQTT_CONFIG_ADDRESS_LENGTH) 
    {
        Print("Init Manager", "MQTT address too long. Please check the src for any descrepancies.");
        return MQTT_DETAILS_TOBIG;
    }

    memset(mqtt_details.address, 0, MQTT_CONFIG_ADDRESS_LENGTH);
    strncpy(mqtt_details.address, address, addressBufferLength);

    Print("Init Manager", "MQTT address set");
    return MQTT_DETAILS_OK;
}

int set_mqtt_config_username(char username[], int usernameBufferLength)
{
    if (usernameBufferLength > MQTT_CONFIG_USERNAME_LENGTH) 
    {
        Print("Init Manager", "MQTT username too long. Please check the src for any descrepancies.");
        return MQTT_DETAILS_TOBIG;
    }

    memset(mqtt_details.username, 0, MQTT_CONFIG_USERNAME_LENGTH);
    strncpy(mqtt_details.username, username, usernameBufferLength);

    Print("Init Manager", "MQTT username set");
    return MQTT_DETAILS_OK;
}

char* get_mqtt_config_address_nullterm_safe(void)
{
    char *copy = calloc(MQTT_CONFIG_ADDRESS_LENGTH + 1, sizeof(char));
    memset(copy, '\0', MQTT_CONFIG_ADDRESS_LENGTH + 1);
    strncpy(copy, mqtt_details.address, MQTT_CONFIG_ADDRESS_LENGTH);
    return copy;
}

char* get_mqtt_config_username_nullterm_safe(void)
{
    char *copy = calloc(MQTT_CONFIG_USERNAME_LENGTH + 1, sizeof(char));
    memset(copy, '\0', MQTT_CONFIG_USERNAME_LENGTH + 1);
    strncpy(copy, mqtt_details.username, MQTT_CONFIG_USERNAME_LENGTH);
    return copy;
}