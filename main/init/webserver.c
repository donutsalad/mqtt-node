#include "ap_config.h"

#include <stdio.h>

#include <freertos/event_groups.h>

#include <esp_http_server.h>

//TODO: Watch for success and errors that aren't handled.
static EventGroupHandle_t webserver_event_group;

void wait_for_wifi_config(void)
{
    xEventGroupWaitBits(
        webserver_event_group,
        SERVER_CONFIG_OK,
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
        SERVER_SHUTDOWN,
        pdFALSE, pdFALSE,
        portMAX_DELAY
    );
}

const char* unknown_entity   = "UNKNOWN ENTITY";
const char* wifi_config_page = WIFI_CONFIG_HTML_WIFI_DETAILS;
const char* wifi_stylesheet  = WIFI_CONFIG_HTML_PAGE_CSSFILE;

static esp_err_t webserver_get_entity(httpd_req_t *req, wifi_entity_t entity)
{
    const char** resource = NULL;

    switch(entity)
    {
        case WIFI_ENTITY_WIFI_PAGE:
            resource = &wifi_config_page;
            break;

        case WIFI_ENTITY_PAGE_CSS:
            resource = &wifi_stylesheet;
            break;

        default:
            resource = &unknown_entity;
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

static esp_err_t webserver_get_root(httpd_req_t *req)
{
    Print("Webserver", "Request for config page received, serving...");
    return webserver_get_entity(req, WIFI_ENTITY_WIFI_PAGE);
}

static esp_err_t webserver_get_style(httpd_req_t *req)
{
    Print("Webserver", "Request for stylesheet received, serving...");
    return webserver_get_entity(req, WIFI_ENTITY_PAGE_CSS);
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
        return WIFI_CONFIG_BADFORMAT;
    }

    walkalong_conversion(ssid, ssidLen);
    walkalong_conversion(pass, passLen);

    return WIFI_CONFIG_OK;
}

static const int SSIDLength = WIFI_SSID_BUFFER_MAX; static const int PASSLength = WIFI_PASS_BUFFER_MAX;
static int config_set_handler(char data[], int length)
{
    char ssid[SSIDLength];
    char pass[PASSLength];

    //Now Redundant.
    //memset(ssid, '\0', sizeof(ssid));
    //memset(pass, '\0', sizeof(pass));

    switch(parse_post_parameters(data, length, ssid, SSIDLength, pass, PASSLength))
    {
        case WIFI_CONFIG_OK:
            Print("Webserver", "POST data parsed successfully!");
            break;

        case WIFI_CONFIG_BADFORMAT:
            Print("Webserver", "Bad format for POST data! Aborting set...");
            return WIFI_CONFIG_BADFORMAT;

        default:
            Print("Webserver", "Unhandled error while parsing POST data! Aborting set...");
            return WIFI_CONFIG_UNSAVABLE;
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
            return WIFI_CONFIG_UNSAVABLE;

        default:
            Print("Webserver", "Unhandled error while setting new wifi config! Aborting set...");
            return WIFI_CONFIG_UNSAVABLE;
    }

    Print("Webserver", "Setting server config saved flag.");

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_CONFIG_OK
    );

    return WIFI_CONFIG_OK;
}

static esp_err_t webserver_set_config(httpd_req_t *req)
{
    char buffer[250];
    if(req->content_len < 1) return ESP_FAIL;
    else if(req->content_len > 250) return ESP_FAIL;

    int length = httpd_req_recv(req, buffer, req->content_len);
    if(length <= 0) return ESP_FAIL;
    switch(config_set_handler(buffer, req->content_len))
    {
        case WIFI_CONFIG_OK:
            Print("Webserver", "Config event was successful!");
            switch(httpd_resp_send(req, WIFI_CONFIG_SET, HTTPD_RESP_USE_STRLEN))
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
        
        case WIFI_CONFIG_BADFORMAT:
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

        case WIFI_CONFIG_UNSAVABLE:
            Print("Webserver", "Internal server error during /set handling! Aborting...");
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

    xEventGroupSetBits(
        webserver_event_group, 
        SERVER_SET_CONFIG_DONE
    );

    return ESP_OK;
}

static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = webserver_get_root,
    .user_ctx  = NULL
};

static const httpd_uri_t config_uri = {
    .uri       = "/set",
    .method    = HTTP_POST,
    .handler   = webserver_set_config,
    .user_ctx  = NULL
};

static const httpd_uri_t style_uri = {
    .uri       = "/style.css",
    .method    = HTTP_GET,
    .handler   = webserver_get_style,
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

    switch(httpd_register_uri_handler(server, &root_uri))
    {
        case ESP_OK:
            break;
        
        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst registering root uri handle? Unsure whow we got this error... Aborting...");
            return NULL;
            //Eventually attempt to handle this since we're all the way to setting up a webserver

        case ESP_ERR_HTTPD_HANDLER_EXISTS:
            Print("Webserver", "Uri handler already exists! Aborting...");
            return NULL;
            //TODO: Deregister whatever handler is there and register this one in its place

        case ESP_ERR_HTTPD_HANDLERS_FULL:
            Print("Webserver", "Too many handlers registered! Aborting...");
            return NULL;

        default:
            Print("Webserver", "Unhandled error while registering root uri handle! Aborting...");
            return NULL;
    }

    switch(httpd_register_uri_handler(server, &style_uri))
    {
        case ESP_OK:
            break;
        
        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst registering stylesheet uri handle? Unsure whow we got this error... Aborting...");
            return NULL;
            //Eventually attempt to handle this since we're all the way to setting up a webserver

        case ESP_ERR_HTTPD_HANDLER_EXISTS:
            Print("Webserver", "Uri handler already exists! Aborting...");
            return NULL;
            //TODO: Deregister whatever handler is there and register this one in its place

        case ESP_ERR_HTTPD_HANDLERS_FULL:
            Print("Webserver", "Too many handlers registered! Aborting...");
            return NULL;

        default:
            Print("Webserver", "Unhandled error while registering stylesheet uri handle! Aborting...");
            return NULL;
    }

    switch(httpd_register_uri_handler(server, &config_uri))
    {
        case ESP_OK:
            break;
        
        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst registering config uri handle? Unsure whow we got this error... Aborting...");
            return NULL;
            //Eventually attempt to handle this since we're all the way to setting up a webserver

        case ESP_ERR_HTTPD_HANDLER_EXISTS:
            Print("Webserver", "Uri handler already exists! Aborting...");
            return NULL;
            //TODO: Deregister whatever handler is there and register this one in its place

        case ESP_ERR_HTTPD_HANDLERS_FULL:
            Print("Webserver", "Too many handlers registered! Aborting...");
            return NULL;

        default:
            Print("Webserver", "Unhandled error while registering root config handle! Aborting...");
            return NULL;
    }

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
        SERVER_SET_CONFIG_DONE, 
        pdFALSE, pdFALSE, 
        portMAX_DELAY
    );

    //My reason for this is to stop any new config set requests from being made whilst shutting down
    //I'm not sure if it's really necessary, but it's a good idea to be safe until I find out for sure.
    //Honestly you could probably block on SERVER_SET_CONFIG_DONE but I realised that later on, so LOL

    int resultant = SERVER_OK;

    switch(httpd_unregister_uri(webserver, "/"))
    {
        case ESP_OK:
            break;

        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst unregistering root uri handle? Unsure whow we got this error, but it's not important. Continuing...");
            resultant = SERVER_IGNORE;
            break;

        case ESP_ERR_NOT_FOUND:
            Print("Webserver", "Uri handler not found! No matter, destroying webserver anyway. Continuing...");
            resultant = SERVER_IGNORE;
            break;

        default:
            Print("Webserver", "Unhandled error while unregistering root uri handle! Aborting...");
            Print("Webserver", "If error persists, consider breaking on the following LoC due to it being non-critical - however please submit an issue on the repository.");
            return SERVER_FATAL;
    }

    switch(httpd_unregister_uri(webserver, "/style.css"))
    {
        case ESP_OK:
            break;

        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst unregistering stylesheet uri handle? Unsure whow we got this error, but it's not important. Continuing...");
            resultant = SERVER_IGNORE;
            break;

        case ESP_ERR_NOT_FOUND:
            Print("Webserver", "Uri handler not found! No matter, destroying webserver anyway. Continuing...");
            resultant = SERVER_IGNORE;
            break;

        default:
            Print("Webserver", "Unhandled error while unregistering stylesheet uri handle! Aborting...");
            Print("Webserver", "If error persists, consider breaking on the following LoC due to it being non-critical - however please submit an issue on the repository.");
            return SERVER_FATAL;
    }

    switch(httpd_unregister_uri(webserver, "/set"))
    {
        case ESP_OK:
            break;

        case ESP_ERR_INVALID_ARG:
            Print("Webserver", "Invalid argument whilst unregistering config uri handle? Unsure whow we got this error, but it's not important. Continuing...");
            resultant = SERVER_IGNORE;
            break;

        case ESP_ERR_NOT_FOUND:
            Print("Webserver", "Uri handler not found! No matter, destroying webserver anyway. Continuing...");
            resultant = SERVER_IGNORE;
            break;

        default:
            Print("Webserver", "Unhandled error while unregistering config uri handle! Aborting...");
            Print("Webserver", "If error persists, consider breaking on the following LoC due to it being non-critical - however please submit an issue on the repository.");
            return SERVER_FATAL;
    }

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
        SERVER_SHUTDOWN
    );

    Print("Webserver", "Webserver shutdown complete.");

    return resultant;
}