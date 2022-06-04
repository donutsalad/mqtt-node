#include "ap_config.h"

#include <stdio.h>

#include <freertos/event_groups.h>

#include <esp_http_server.h>

//TODO: Watch for success and errors that aren't handled.
static EventGroupHandle_t webserver_event_group;

void wait_for_config_okay(void)
{
    xEventGroupWaitBits(
        webserver_event_group,
        SERVER_CONFIG_OKAY,
        pdFALSE, pdFALSE,
        portMAX_DELAY
    );
}

void wait_for_webserver_start(void)
{
    xEventGroupWaitBits(
        webserver_event_group, 
        SERVER_READY, 
        pdFALSE, pdFALSE, 
        portMAX_DELAY
    );
}

void wait_for_shutdown_completion(void)
{
    xEventGroupWaitBits(
        webserver_event_group,
        SERVER_STOP_FINISH,
        pdFALSE, pdFALSE,
        portMAX_DELAY
    );
}

const char* unknown_entity   = "UNKNOWN ENTITY";

static const char* wifi_config_page = WIFI_CONFIG_HTML_WIFI_DETAILS;
static const char* wifi_set_ok_page = WIFI_CONFIG_HTML_WIFI_CONFIRM;
static const char* mqtt_config_page = WIFI_CONFIG_HTML_MQTT_DETAILS;
static const char* mqtt_set_ok_page = WIFI_CONFIG_HTML_MQTT_CONFIRM;
static const char* mode_config_page = WIFI_CONFIG_HTML_MODE_DETAILS;
static const char* boot_notyet_page = WIFI_CONFIG_HTML_DENY_BOOTNOW;
static const char* done_config_page = WIFI_CONFIG_HTML_DONE_CONFIRM;
static const char* uerr_fnferr_page = WIFI_CONFIG_HTML_E400_ERRPAGE;
static const char* uerr_ivferr_page = WIFI_CONFIG_HTML_E400_ERRPAGE;
static const char* ierr_inserr_page = WIFI_CONFIG_HTML_E500_ERRPAGE;
static const char* only_styles_file = WIFI_CONFIG_HTML_ONLY_CSSFILE;

static esp_err_t webserver_get_entity(httpd_req_t *req, wifi_entity_t entity)
{
    const char** resource = NULL;

    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & SERVER_SHUTDOWN_OK) {
        return ESP_FAIL;
    }
    else if(bits & SERVER_FCSS_SERVED) {
        xEventGroupSetBits(webserver_event_group, SERVER_SHUTDOWN_OK);
        return ESP_FAIL;
    }
    else if(bits & SERVER_FINAL_SERVE) {
        if(entity != WIFI_ENTITY_PAGE_CSS) {
            return ESP_FAIL;
        }
    }

    switch(entity)
    {
        case WIFI_ENTITY_WIFI_PAGE:
            resource = &wifi_config_page;
            break;

        case WIFI_ENTITY_WIFI_DONE:
            resource = &wifi_set_ok_page;
            break;

        case WIFI_ENTITY_MQTT_PAGE:
            resource = &mqtt_config_page;
            break;

        case WIFI_ENTITY_MQTT_DONE:
            resource = &mqtt_set_ok_page;
            break;

        case WIFI_ENTITY_DENY_BOOT:
            resource = &boot_notyet_page;
            break;

        case WIFI_ENTITY_MODE_PAGE:
            resource = &mode_config_page;
            break;

        case WIFI_ENTITY_DONE_PAGE:
            resource = &done_config_page;
            break;

        case WIFI_ENTITY_E400_PAGE:
            resource = &uerr_ivferr_page;
            break;

        case WIFI_ENTITY_E404_PAGE:
            resource = &uerr_fnferr_page;
            break;

        case WIFI_ENTITY_E500_PAGE:
            resource = &ierr_inserr_page;
            break;

        case WIFI_ENTITY_PAGE_CSS:
            resource = &only_styles_file;
            break;

        default:
            resource = &unknown_entity;
            break;
    }


    switch(httpd_resp_set_type(req, "text/html"))
    {
        case ESP_OK:
            break;

        case ESP_ERR_HTTPD_INVALID_REQ:
            Print("Webserver", "Invalid request pointer handed to argument?? Aborting...");
            return ESP_FAIL;

        case ESP_ERR_INVALID_ARG:
        default:
            Print("Webserver", "Unhandled error while setting response type! Aborting...");
            return ESP_FAIL;
    }

    switch(httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"))
    {
        case ESP_OK:
            break;

        case ESP_ERR_HTTPD_INVALID_REQ:
            Print("Webserver", "Invalid request pointer handed to argument?? Aborting...");
            return ESP_FAIL;

        case ESP_ERR_HTTPD_RESP_HDR:
            Print("Webserver", "To many headers in request! Please ensure check config files! Aborting...");
            return ESP_FAIL;

        case ESP_ERR_INVALID_ARG:
        default:
            Print("Webserver", "Unhandled error while setting response header! Aborting...");
            return ESP_FAIL;
    }

    switch(httpd_resp_send(req, *resource, HTTPD_RESP_USE_STRLEN))
    {
        case ESP_ERR_HTTPD_INVALID_REQ:
            Print("Webserver", "Invalid request pointer handed to argument?? Aborting...");
            return ESP_FAIL;

        case HTTPD_SOCK_ERR_TIMEOUT:
            Print("Webserver", "Socket timeout! Aborting...");
            return ESP_FAIL;

        case HTTPD_SOCK_ERR_INVALID:
            Print("Webserver", "Invalid argument? Unsure whow we got this error... Aborting...");
            return ESP_FAIL;

        case HTTPD_SOCK_ERR_FAIL:
            Print("Webserver", "Unrecoverable socket error! Aborting...");
            return ESP_FAIL;

        default:
            Print("Webserver", "Served config page!");
            return ESP_OK;
    }
}

static esp_err_t webserver_get_wifi(httpd_req_t *req)
{
    Print("Webserver", "Request for config page received, serving...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & SERVER_WIFI_CONFIG)
    {
        Print("Webserver", "Wifi config already set, serving...");
        return webserver_get_entity(req, WIFI_ENTITY_WIFI_DONE);
    }
    return webserver_get_entity(req, WIFI_ENTITY_WIFI_PAGE);
}

static esp_err_t webserver_get_mqtt(httpd_req_t *req)
{
    Print("Webserver", "Request for config page received, serving...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & SERVER_MQTT_CONFIG)
    {
        Print("Webserver", "MQTT config already set, serving...");
        return webserver_get_entity(req, WIFI_ENTITY_MQTT_DONE);
    }
    return webserver_get_entity(req, WIFI_ENTITY_MQTT_PAGE);
}

static esp_err_t webserver_get_boot(httpd_req_t *req)
{
    Print("Webserver", "Request for config page received, routing...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & SERVER_BOOT_CONFIG)
    {
        Print("Webserver", "Everything is set! Igonring request in the interum, server should be shutdown soon.");
        return ESP_FAIL;
    }

    if(bits & SERVER_WIFI_CONFIG)
    {
        if(bits & SERVER_MQTT_CONFIG)
        {
            return webserver_get_entity(req, WIFI_ENTITY_MODE_PAGE);
        }
        Print("Webserver", "MQTT config not yet set! Serving ping root page...");
        return webserver_get_entity(req, WIFI_ENTITY_DENY_BOOT);
    }

    Print("Webserver", "WiFi config are not yet set! Serving ping root page...");
    return webserver_get_entity(req, WIFI_ENTITY_DENY_BOOT);
}

static esp_err_t webserver_get_style(httpd_req_t *req)
{
    Print("Webserver", "Request for stylesheet received, serving...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    //Gauranteed serve of CSS before shutdown (bar timeout)
    if(bits & SERVER_FINAL_SERVE) {
        esp_err_t result = webserver_get_entity(req, WIFI_ENTITY_PAGE_CSS);
        xEventGroupSetBits(webserver_event_group, SERVER_FCSS_SERVED | SERVER_SHUTDOWN_OK);
        return result;
    }
    return webserver_get_entity(req, WIFI_ENTITY_PAGE_CSS);
}

static esp_err_t webserver_get_root(httpd_req_t *req)
{
    Print("Webserver", "Routing root request...");

    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & SERVER_WIFI_CONFIG)
    {
        if(bits & SERVER_MQTT_CONFIG)
        {
            if(bits & SERVER_BOOT_CONFIG)
            {
                Print("Webserver", "Everything is set! Igonring request in the interum, server should be shutdown soon.");
                return ESP_OK;
            }

            Print("Webserver", "Mode is not set! Serving mode config page...");
            return webserver_get_boot(req);
        }

        Print("Webserver", "MQTT config is not set! Serving mqtt config page...");
        return webserver_get_mqtt(req);
    }

    Print("Webserver", "No settings set, sending wifi config page...");
    return webserver_get_wifi(req);
}

//ASCII hex to raw hex nibble downconversion
//i.e. "A" -> 0x0A
static inline char hex_value(char x) {
    char buffer = (((x / 16) - 3) * 10) + (x % 16);
    return (buffer > 9) ? --buffer : buffer;
}

//ASCII from upper & lower nibble combination
//i.e. (0x0A, 0xB0) -> 0xAB
static inline char hex_to_ascii(char a, char b) {
    return (hex_value(a) * 16) + hex_value(b);
}

//In place URL decoding
//i.e. "test%2Ftest" -> "test/test"
//NOTE: correctly shifts nullterm '\0' twice for every %XY block,
//      however does not blank out remainder of buffer.
static inline void walkalong_conversion(char buffer[], int length)
{
    int conversions = 0; int i = 0; for(; i < length; i++)
    {
        //Prevent overflow from truncated buffer (i.e. ending in % or %x)
        if(i + (conversions * 2) > length)
        {
            break;
        }

        switch(buffer[i + (conversions * 2)])
        {
            case '%':
                buffer[i + (conversions * 2)] = hex_to_ascii(buffer[i + (conversions * 2) + 1], buffer[i + (conversions * 2) + 2]);
                conversions++;
                break;

            case '+':
                buffer[i] = ' ';
                break;

            default:
                buffer[i] = buffer[i + (conversions * 2)];
                break;
        }
    }
    buffer[++i] = '\0';
}

//TODO: Standardise the a & b params.
//TODO: NULL EXCEPTION HANDLING
//Expected format:
//a=<ssid>&b=<pass> in URL encoding
static int parse_post_parameters(char raw[], int length, char ssid[], int ssidMaxLen, char pass[], int passMaxLen)
{
    for(int i = 0; i < length; i++)
    {
        //Copilot wrote this for me, the future is now! XD
        if(raw[i] == 'a' && raw[i + 1] == '=')
        {
            int j = 0;
            for(i += 2; i < length && raw[i] != '&' && j < ssidMaxLen; i++)
            {
                ssid[j++] = raw[i];
            }
            ssid[j] = '\0';
        }
        else if(raw[i] == 'b' && raw[i + 1] == '=')
        {
            int j = 0;
            for(i += 2; i < length && raw[i] != '&' && j < passMaxLen; i++)
            {
                pass[j++] = raw[i];
            }
            pass[j] = '\0';
        }
        //Very gamer indeed >:3
    }

    int ssidLen = strlen(ssid);
    int passLen = strlen(pass);

    if (ssidLen == 0 || passLen == 0)
    {
        return SERVER_CONFIG_BADFORMAT;
    }

    walkalong_conversion(ssid, ssidLen);
    walkalong_conversion(pass, passLen);

    return SERVER_CONFIG_OK;
}

static const int SSIDLength = WIFI_SSID_BUFFER_MAX; static const int PASSLength = WIFI_PASS_BUFFER_MAX;
static int config_wifi_set_handler(char data[], int length)
{
    char ssid[SSIDLength];
    char pass[PASSLength];

    //Now Redundant.
    //memset(ssid, '\0', sizeof(ssid));
    //memset(pass, '\0', sizeof(pass));

    switch(parse_post_parameters(data, length, ssid, SSIDLength, pass, PASSLength))
    {
        case SERVER_CONFIG_OK:
            Print("Webserver", "POST data parsed successfully!");
            break;

        case SERVER_CONFIG_BADFORMAT:
            Print("Webserver", "Bad format for POST data! Aborting set...");
            return SERVER_CONFIG_BADFORMAT;

        default:
            Print("Webserver", "Unhandled error while parsing POST data! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Setting new wifi config...");
    
    switch(set_wifi_details(ssid, SSIDLength, pass, PASSLength))
    {
        case WIFI_DETAILS_OK:
            Print("Webserver", "New wifi config set successfully!");
            break;

        case WIFI_DETAILS_TOBIG:
            Print("Webserver", "New wifi config buffers are too big! Aborting set...");
            Print("Webserver", "Please check the buffers in the src and open an issue on github if there is a mismatch!");
            return SERVER_CONFIG_UNSAVABLE;

        default:
            Print("Webserver", "Unhandled error while setting new wifi config! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Setting server config saved flag.");

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_WIFI_CONFIG
    );

    return SERVER_CONFIG_OK;
}

static const int ENDPOINTLength = MQTT_ENDPOINT_BUFFER; static const int USERNAMELength = MQTT_USERNAME_BUFFER;
static int config_mqtt_set_handler(char data[], int length)
{
    char endpoint[ENDPOINTLength];
    char username[USERNAMELength];

    //Now Redundant.
    //memset(ssid, '\0', sizeof(ssid));
    //memset(pass, '\0', sizeof(pass));

    switch(parse_post_parameters(data, length, endpoint, ENDPOINTLength, username, USERNAMELength))
    {
        case SERVER_CONFIG_OK:
            Print("Webserver", "POST data parsed successfully!");
            break;

        case SERVER_CONFIG_BADFORMAT:
            Print("Webserver", "Bad format for POST data! Aborting set...");
            return SERVER_CONFIG_BADFORMAT;

        default:
            Print("Webserver", "Unhandled error while parsing POST data! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Setting new mqtt config...");
    
    switch(set_mqtt_config(endpoint, ENDPOINTLength, username, USERNAMELength))
    {
        case MQTT_DETAILS_OK:
            Print("Webserver", "New mqtt config set successfully!");
            break;

        case MQTT_DETAILS_TOBIG:
            Print("Webserver", "New mqtt config buffers are too big! Aborting set...");
            Print("Webserver", "Please check the buffers in the src and open an issue on github if there is a mismatch!");
            return SERVER_CONFIG_UNSAVABLE;

        default:
            Print("Webserver", "Unhandled error while setting new mqtt config! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Setting mqtt config saved flag.");

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_MQTT_CONFIG
    );

    return SERVER_CONFIG_OK;
}

static const int BOOTMODELength = BOOT_MODE_BUFFER; static const int BOOTDELAYLength = BOOT_DELAY_BUFFER;
static int config_boot_save_commons(char data[], int length)
{
    char bootmode[BOOTMODELength];
    char bootdelay[BOOTDELAYLength];

    switch(parse_post_parameters(data, length, bootmode, ENDPOINTLength, bootdelay, USERNAMELength))
    {
        case SERVER_CONFIG_OK:
            Print("Webserver", "POST data parsed successfully!");
            break;

        case SERVER_CONFIG_BADFORMAT:
            Print("Webserver", "Bad format for POST data! Aborting set...");
            return SERVER_CONFIG_BADFORMAT;

        default:
            Print("Webserver", "Unhandled error while parsing POST data! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Setting new boot config...");

    int bootmode_int = boot_config_mode_from_string(bootmode);
    int bootdelay_int = boot_config_delay_from_string(bootdelay);

    switch(set_boot_config(bootmode_int, bootdelay_int))
    {
        case BOOT_DETAILS_OK:
            Print("Webserver", "New boot config set successfully!");
            break;

        case BOOT_DETAILS_BADFORMAT:
            Print("Webserver", "Bad format for boot config! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;

        default:
            Print("Webserver", "Unhandled error while setting new boot config! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    return SERVER_CONFIG_OK;
}

static int config_boot_set_handler(char data[], int length)
{
    switch(config_boot_save_commons(data, length))
    {
        case SERVER_CONFIG_OK:
            Print("Webserver", "Boot config parsed and loaded successfully!");
            break;

        case SERVER_CONFIG_BADFORMAT:
            Print("Webserver", "Bad format for boot config! Aborting set...");
            return SERVER_CONFIG_BADFORMAT;

        default:
            Print("Webserver", "Unhandled error while parsing boot config! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Setting boot config saved flag.");

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_BOOT_CONFIG | SERVER_CONFIG_OKAY
    );

    return SERVER_CONFIG_OK;
}

static int config_save_set_handler(char data[], int length)
{
    switch(config_boot_save_commons(data, length))
    {
        case SERVER_CONFIG_OK:
            Print("Webserver", "Boot config parsed and loaded successfully!");
            break;

        case SERVER_CONFIG_BADFORMAT:
            Print("Webserver", "Bad format for boot config! Aborting set...");
            return SERVER_CONFIG_BADFORMAT;

        default:
            Print("Webserver", "Unhandled error while parsing boot config! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    switch(update_restore_config(wifi_details(), mqtt_config(), boot_config()))
    {
        case UPDATE_RESTORE_OK:
            Print("Webserver", "Config saved successfully!");
            break;

        default:
            Print("Webserver", "Config save failed! Aborting set...");
            return SERVER_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Saving boot config saved flag.");

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_BOOT_CONFIG | SERVER_CONFIG_OKAY
    );

    return SERVER_CONFIG_OK;
}

static const char* get_wifi_set_page() { return wifi_set_ok_page; }
static const char* get_mqtt_set_page() { return mqtt_set_ok_page; }

//TODO: RENDER ERROR HANDLING
//Is calloc a better option here?
//We could free immediately after served. 
//const char* boot_buffer = boot_set_ok_page();
static char rendered_boot_page[1024];

static const char* confirmation_page_boot_template = WIFI_CONFIG_HTML_DONE_CONFIRM;
static const char* confirmation_page_save_template = WIFI_CONFIG_HTML_SAVE_CONFIRM;

static const char* render_page(render_page_id_t page_id)
{
    const char* *page_template = NULL;
    switch(page_id)
    {
        case RENDER_PAGE_ID_BOOT:
            page_template = &confirmation_page_boot_template;
            break;
        
        case RENDER_PAGE_ID_SAVE:
            page_template = &confirmation_page_save_template;
            break;

        default:
            Print("Webserver", "Unknown page id!");
            return "UNKNOWN PAGE RENDER REQUEST ID";
    }

    wifi_connection_details_t *wifi_config = wifi_details();
    mqtt_config_t *mqtt_details = mqtt_config();
    boot_config_t *boot_details = boot_config();

    int format = snprintf(rendered_boot_page, sizeof(rendered_boot_page), *page_template, 
        wifi_config->ssid, wifi_config->pass, 
        mqtt_details->address, mqtt_details->username,
        bootconfig_type_from_int(boot_details->mode), boot_details->delay
    );
    
    if(format < 0)
    {
        Print("Webserver", "Error while rendering boot confirmation page!");
        return "ENCODING ERROR";
    }
    else if(format >= sizeof(rendered_boot_page))
    {
        Print("Webserver", "Confirmation page is too big!");
        return "BUFFER OVERFLOW ERROR";
    }

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_FINAL_SERVE
    );

    Print("Webserver", "Boot confirmation page rendered successfully!");
    return (const char*) rendered_boot_page;
}

static const char* get_boot_page() { return render_page(RENDER_PAGE_ID_BOOT); }
static const char* get_save_page() { return render_page(RENDER_PAGE_ID_SAVE); }

static esp_err_t webserver_set_entity(httpd_req_t *req, wifi_set_code_t set, char* tempDebugMessage)
{
    Print("Webserver", "The following message is the only dynamic message in the following function. Please note the source.");
    Print("Webserver", tempDebugMessage);

    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & (SERVER_SHUTDOWN_OK | SERVER_FINAL_SERVE)) return ESP_FAIL;

    char buffer[250];
    if(req->content_len < 1) return ESP_FAIL;
    else if(req->content_len > 250) return ESP_FAIL;

    int length = httpd_req_recv(req, buffer, req->content_len);
    if(length <= 0) return ESP_FAIL;

    int (*set_handler)(char[], int) = NULL;
    const char* (*ok_responce)(void) = NULL;

    switch(set)
    {
        case WIFI_SET_CODE_WIFI:
            set_handler = &config_wifi_set_handler;
            ok_responce = &get_wifi_set_page;
            break;

        case WIFI_SET_CODE_MQTT:
            set_handler = &config_mqtt_set_handler;
            ok_responce = &get_mqtt_set_page;
            break;

        case WIFI_SET_CODE_MODE:
            set_handler = &config_boot_set_handler;
            ok_responce = &get_boot_page;
            break;

        case WIFI_SET_CODE_SAVE:
            set_handler = &config_save_set_handler;
            ok_responce = &get_save_page;
            break;

        default:
            return ESP_FAIL;
    }

    switch((*set_handler)(buffer, req->content_len))
    {
        case SERVER_CONFIG_OK:
            Print("Webserver", "Config event was successful!");
            switch(httpd_resp_send(req, (*ok_responce)(), HTTPD_RESP_USE_STRLEN))
            {
                case ESP_ERR_HTTPD_INVALID_REQ:
                case HTTPD_SOCK_ERR_TIMEOUT:
                case HTTPD_SOCK_ERR_INVALID:
                case HTTPD_SOCK_ERR_FAIL:
                    Print("Webserver", "Unable to send set config response!");
                    Print("Webserver", "Although non-critical it could be indicative of an underlying issue. Please check src or open a github issue!");
                    break;

                default:
                    Print("Webserver", "Set config reponce successfuly sent!");
                    break;
            }
            break;
        
        case SERVER_CONFIG_BADFORMAT:
            Print("Webserver", "Bad format for POST data! Aborting set...");
            //TODO: Bad format page (?)
            switch(httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad format for POST data!"))
            {
                case ESP_ERR_INVALID_ARG:
                case ESP_ERR_HTTPD_RESP_SEND:
                case ESP_ERR_HTTPD_INVALID_REQ:
                    Print("Webserver", "Unable to send bad format (400) response!");
                    Print("Webserver", "Although non-critical it could be indicative of an underlying issue. Please check src or open a github issue!");
                    break;

                case ESP_OK:
                    Print("Webserver", "Bad format (400) response sent!");
                    break;
            }
            return ESP_FAIL;

        case SERVER_CONFIG_UNSAVABLE:
            Print("Webserver", "Internal server error during config set handling! Aborting...");
            switch(httpd_resp_send_500(req))
            {
                case ESP_ERR_INVALID_ARG:
                case ESP_ERR_HTTPD_RESP_SEND:
                case ESP_ERR_HTTPD_INVALID_REQ:
                    Print("Webserver", "Unable to send 500 response!");
                    Print("Webserver", "Although non-critical it could be indicative of an underlying issue. Please check src or open a github issue!");
                    break;

                default:
                    Print("Webserver", "500 response sent!");
                    break;
            }
            return ESP_FAIL;
    }

    Print("Webserver", "Set config event was successful! Setting finished bit.");
    return ESP_OK;
}

static esp_err_t webserver_set_wifi(httpd_req_t *req)
{
    Print("Webserver", "POST Request for WiFi config recieved...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & SERVER_WIFI_CONFIG)
    {
        Print("Webserver", "Wifi config already set! Servering already done page...");
        return webserver_get_entity(req, WIFI_ENTITY_WIFI_DONE);
    }
    return webserver_set_entity(req, WIFI_SET_CODE_WIFI, "This URI is: SET WIFI");
}

static esp_err_t webserver_set_mqtt(httpd_req_t *req)
{
    Print("Webserver", "POST Request for MQTT config recieved...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & SERVER_MQTT_CONFIG)
    {
        Print("Webserver", "MQTT config already set! Servering already done page...");
        return webserver_get_entity(req, WIFI_ENTITY_MQTT_DONE);
    }
    return webserver_set_entity(req, WIFI_SET_CODE_MQTT, "This URI is: SET MQTT");
}

static esp_err_t webserver_set_boot(httpd_req_t *req)
{
    Print("Webserver", "POST Request for BOOT config recieved...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & (SERVER_WIFI_CONFIG | SERVER_MQTT_CONFIG))
    {
        return webserver_set_entity(req, WIFI_SET_CODE_MODE, "This URI is: SET BOOT");
    }

    Print("Webserver", "Other configs are not yet set! Serving ping root page...");
    return webserver_get_entity(req, WIFI_ENTITY_DENY_BOOT);
}

static esp_err_t webserver_set_save(httpd_req_t *req)
{
    Print("Webserver", "POST Request for SAVE config recieved...");
    EventBits_t bits = xEventGroupGetBits(webserver_event_group);
    if(bits & (SERVER_WIFI_CONFIG | SERVER_MQTT_CONFIG))
    {
        return webserver_set_entity(req, WIFI_SET_CODE_SAVE, "This URI is: SET SAVE");
    }

    Print("Webserver", "Other configs are not yet set! Serving ping root page...");
    return webserver_get_entity(req, WIFI_ENTITY_DENY_BOOT);
}

static int attempt_register_uri_handler(httpd_handle_t server, const httpd_uri_t *uri, char* tempDebugMessage)
{
    Print("Webserver", "The following message is the only dynamic message in the following function. Please note the source.");
    Print("Webserver", tempDebugMessage);
    switch(httpd_register_uri_handler(server, uri))
    {
        case ESP_OK:
            return 0;
        
        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst registering afforementioned uri handle? Unsure whow we got this error... Aborting...");
            return 1;
            //Eventually attempt to handle this since we're all the way to setting up a webserver

        case ESP_ERR_HTTPD_HANDLER_EXISTS:
            Print("Webserver", "Uri handler already exists! Aborting...");
            return 1;
            //TODO: Deregister whatever handler is there and register this one in its place

        case ESP_ERR_HTTPD_HANDLERS_FULL:
            Print("Webserver", "Too many handlers registered! Aborting...");
            return 1;

        default:
            Print("Webserver", "Unhandled error while registering afforementioned uri handle! Aborting...");
            return 1;
    }
}

static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = webserver_get_root,
    .user_ctx  = NULL
};

static const httpd_uri_t style_uri = {
    .uri       = "/style.css",
    .method    = HTTP_GET,
    .handler   = webserver_get_style,
    .user_ctx  = NULL
};

static const httpd_uri_t wifi_get_uri = {
    .uri       = "/wifi",
    .method    = HTTP_GET,
    .handler   = webserver_get_wifi,
    .user_ctx  = NULL
};

static const httpd_uri_t mqtt_get_uri = {
    .uri       = "/mqtt",
    .method    = HTTP_GET,
    .handler   = webserver_get_mqtt,
    .user_ctx  = NULL
};

static const httpd_uri_t boot_get_uri = {
    .uri       = "/boot",
    .method    = HTTP_GET,
    .handler   = webserver_get_boot,
    .user_ctx  = NULL
};

static const httpd_uri_t wifi_set_uri = {
    .uri       = "/wifi",
    .method    = HTTP_POST,
    .handler   = webserver_set_wifi,
    .user_ctx  = NULL
};

static const httpd_uri_t mqtt_set_uri = {
    .uri       = "/mqtt",
    .method    = HTTP_POST,
    .handler   = webserver_set_mqtt,
    .user_ctx  = NULL
};

static const httpd_uri_t boot_set_uri = {
    .uri       = "/boot",
    .method    = HTTP_POST,
    .handler   = webserver_set_boot,
    .user_ctx  = NULL
};

static const httpd_uri_t save_set_uri = {
    .uri       = "/save",
    .method    = HTTP_POST,
    .handler   = webserver_set_save,
    .user_ctx  = NULL
};

//TODO: Add Error Handlers

//TODO: STREAMLINE THE REPEATING SWITCHBLOCKS
static httpd_handle_t create_webserver(void)
{
    if(webserver_event_group == NULL)
    {
        Print("Webserver", "Warning: Event group not created! Creating new one...");
        Print("Webserver", "This is likely due to a previous error. Please check src or open a github issue!");
        webserver_event_group = xEventGroupCreate();
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    httpd_handle_t server = NULL;

    switch(httpd_start(&server, &config))
    {
        case ESP_OK:
            break;

        case ESP_ERR_HTTPD_ALLOC_MEM:
            Print("Webserver", "Failed to allocate memory for webserver! Aborting...");
            return NULL;

        case ESP_ERR_HTTPD_TASK:
            Print("Webserver", "Failed to create webserver task! Aborting...");
            return NULL;

        case ESP_ERR_INVALID_ARG:
        default:
            Print("Webserver", "Unhandled error while starting webserver! Aborting...");
            return NULL;
    }

    if(attempt_register_uri_handler(server, &root_uri, "This URI is: ROOT")) return NULL;
    if(attempt_register_uri_handler(server, &style_uri, "This URI is: STYLE")) return NULL;

    if(attempt_register_uri_handler(server, &wifi_get_uri, "This URI is: WIFI")) return NULL;
    if(attempt_register_uri_handler(server, &mqtt_get_uri, "This URI is: MQTT")) return NULL;
    if(attempt_register_uri_handler(server, &boot_get_uri, "This URI is: BOOT")) return NULL;

    if(attempt_register_uri_handler(server, &wifi_set_uri, "This URI is: WIFI")) return NULL;
    if(attempt_register_uri_handler(server, &mqtt_set_uri, "This URI is: MQTT")) return NULL;
    if(attempt_register_uri_handler(server, &boot_set_uri, "This URI is: BOOT")) return NULL;
    if(attempt_register_uri_handler(server, &save_set_uri, "This URI is: SAVE")) return NULL;

    return server;
}

static int destroy_webserver(httpd_handle_t* server)
{
    int resultant = SERVER_IGNORE;

    if (server)
    {
        switch(httpd_stop(*server))
        {
            case ESP_OK:
                break;

            case ESP_ERR_INVALID_ARG:
                Print("Webserver", "Invalid argument whilst stopping webserver? Unsure whow we got this error... Aborting...");
                return SERVER_FATAL;
                //Eventually attempt to handle this since we're all the way to shutting down the access point.

            default:
                Print("Webserver", "Unhandled error while stopping webserver! Aborting...");
                return SERVER_FATAL;
        }
        server = NULL;
        resultant = SERVER_OK;
    }

    return resultant;
}

static httpd_handle_t webserver = NULL;

void SetupWebserverEventListeners(void)
{
    Print("Webserver", "Setting up event group...");
    webserver_event_group = xEventGroupCreate();
}

int InitialiseWebserver(void)
{
    if(webserver != NULL) return SERVER_EXISTS;

    Print("Webserver", "Initialising webserver...");

    webserver = create_webserver();
    if(webserver == NULL) return SERVER_FATAL;

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_READY
    );
    Print("Webserver", "Webserver initialised!");

    return SERVER_OK;
}

static int attempt_deregister_uri(httpd_handle_t server, const char* uri, char* tempDebugMessage)
{
    Print("Webserver", "The following message is the only dynamic message in the following function. Please note the source.");
    Print("Webserver", tempDebugMessage);
    switch(httpd_unregister_uri(server, uri))
    {
        case ESP_OK:
            return SERVER_OK;

        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst unregistering afforementioned uri handle? Unsure whow we got this error, but it's not important. Continuing...");
            return SERVER_IGNORE;

        case ESP_ERR_NOT_FOUND:
            Print("Webserver", "Uri handler not found! No matter, destroying webserver anyway. Continuing...");
            return SERVER_IGNORE;

        default:
            Print("Webserver", "Unhandled error while unregistering afforementioned uri handle! Aborting...");
            Print("Webserver", "If error persists, consider breaking on the following LoC due to it being non-critical - however please submit an issue on the repository.");
            return SERVER_FATAL;
    }
}

int TriggerWebserverClose(void)
{
    Print("Webserver", "Webserver destruction triggered...");

    xEventGroupClearBits(
        webserver_event_group, 
        SERVER_READY
    );

    //In case the server close is triggered before the /set handler is done.
    Print("Webserver", "Waiting until configuration set handler is completed...");
    xEventGroupWaitBits(
        webserver_event_group, 
        SERVER_SHUTDOWN_OK, 
        pdFALSE, pdFALSE, 
        pdMS_TO_TICKS(5000)
    );

    //My reason for this is to stop any new config set requests from being made whilst shutting down
    //I'm not sure if it's really necessary, but it's a good idea to be safe until I find out for sure.
    //Honestly you could probably block on SERVER_SHUTDOWN_OK but I realised that later on, so LOL

    int resultant = SERVER_OK;

    int resultbuf;
    resultbuf = attempt_deregister_uri(webserver, "/", "This URI is: ROOT");
    if(resultbuf == SERVER_FATAL) return SERVER_FATAL;
    else if(resultbuf == SERVER_IGNORE) resultant = SERVER_IGNORE;

    resultbuf = attempt_deregister_uri(webserver, "/wifi", "This URI is: WIFI");
    if(resultbuf == SERVER_FATAL) return SERVER_FATAL;
    else if(resultbuf == SERVER_IGNORE) resultant = SERVER_IGNORE;

    resultbuf = attempt_deregister_uri(webserver, "/mqtt", "This URI is: MQTT");
    if(resultbuf == SERVER_FATAL) return SERVER_FATAL;
    else if(resultbuf == SERVER_IGNORE) resultant = SERVER_IGNORE;

    resultbuf = attempt_deregister_uri(webserver, "/boot", "This URI is: BOOT");
    if(resultbuf == SERVER_FATAL) return SERVER_FATAL;
    else if(resultbuf == SERVER_IGNORE) resultant = SERVER_IGNORE;

    resultbuf = attempt_deregister_uri(webserver, "/save", "This URI is: SAVE");
    if(resultbuf == SERVER_FATAL) return SERVER_FATAL;
    else if(resultbuf == SERVER_IGNORE) resultant = SERVER_IGNORE;

    resultbuf = attempt_deregister_uri(webserver, "/style.css", "This URI is: STYLE");
    if(resultbuf == SERVER_FATAL) return SERVER_FATAL;
    else if(resultbuf == SERVER_IGNORE) resultant = SERVER_IGNORE;

    switch(destroy_webserver(&webserver))
    {
        case SERVER_OK:
            Print("Webserver", "Webserver destroyed successfully.");
            break;

        case SERVER_IGNORE:
            Print("Webserver", "Webserver destroyed successfully, but ignored error. Continuing...");
            resultant = SERVER_IGNORE;
            break;

        case SERVER_FATAL:
            Print("Webserver", "Webserver unable to be destroyed! Aborting...");
            return SERVER_FATAL;
    }

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_STOP_FINISH
    );

    Print("Webserver", "Webserver shutdown complete.");

    return resultant;
}