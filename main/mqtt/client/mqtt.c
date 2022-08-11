#include "mqttclient.h"

#include <mqtt_client.h>

#include "wifistation.h"
#include "datatypes.h"

static esp_mqtt_client_handle_t mqtt_client = NULL;
static EventGroupHandle_t mqtt_event_group = NULL;

EventGroupHandle_t get_mqtt_event_group(void) { return mqtt_event_group; }

static struct MQTT_Callbacks {
    void (*valid_connection)(void);
    void (*unsuccessful)(void);
    void (*ungraceful_shutdown)(void);
} mqtt_callbacks;

static esp_mqtt_client_config_t mqtt_configuration = {
    .host       = MQTT_UNDEFINED_TAG,
    .client_id  = MQTT_UNDEFINED_TAG,
    .port       = MQTT_NODE_PORT,
    .keepalive  = MQTT_NODE_KEEPALIVE,
    .task_prio  = MQTT_NODE_TASK_PRIORITY,
    .task_stack = MQTT_NODE_TASK_STACK,

    //Cap reconnection attempts, prevent reconnection during termination.
    .disable_auto_reconnect = MQTT_MANUAL_RECONNECT
};

static inline void _generate_mqtt_client_configuration(void) {
    //Need to refactor this - mqtt_config() backwards - a bit misnamed.
    mqtt_configuration.host         = mqtt_config()->address;
    mqtt_configuration.client_id    = mqtt_config()->username;
}

static inline EventBits_t _blockfor_hasip(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_HAS_IP,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

static inline EventBits_t _blockfor_lostip(void) {
    return xEventGroupWaitBits(
        get_wifi_event_group(),
        WIFI_STATION_LOST_IP,
        pdFALSE, pdTRUE,
        portMAX_DELAY
    );
}

static TaskHandle_t xInvalidationTask = NULL;
static void Invalidation_Task(void *pvParameters)
{
    //TODO: After AppStack
    //Check if the PCTASK_CALL flag is set
        //If it has been set and the done flag is set, we need to terminate the entire stack.
        //Otherwise block until it's done and do the same.
    //Else
        //Set the terminating flag
        //Call cleanup ourselves

    //Invalidate the stored details
    //Call the callback routine
    //Kill self

    Print("MQTT Invalidation", "Invalidation request - currently not implemented. Exiting");
    xInvalidationTask = NULL;
    vTaskDelete(NULL);
}

static void InvalidateMQTTConfiguration(void)
{
    if(xInvalidationTask != NULL)
    {
        Print("MQTT Client", "Invalidation co-routine already running! Ignoring extraneous invalidation request.");
        return;
    }

    Print("MQTT Client", "MQTT Configuration Invalidation Requested.");

    BaseType_t resultant = xTaskCreate(
        Invalidation_Task,
        "MQTT Invalidation Task",
        MQTT_INVALID_STACK_SIZE,
        NULL,
        MQTT_NODE_TASK_PRIORITY,
        &xInvalidationTask
    );

    if(resultant != pdPASS)
    {
        Print("MQTT Client", "Failed to create invalidation task! If error persists, please open an issue on GitHub.");
        return;
    }
}

static TaskHandle_t xPostConnectionTask = NULL;
static void Post_Connection_Task(void *pvParameters)
{
    EventBits_t event_group_details;

    if(mqtt_event_group == NULL)
    {
        Print("MQTT Client", "Event group not initialized! Aborting connected call and terminating client! Invalid state!!!");
        ShutdownMQTTServices();
        return;
    }
    else
    {
        event_group_details = xEventGroupGetBits(mqtt_event_group);
        if(event_group_details & MQTT_EVENT_TERMINATING)
        {
            Print("MQTT Client", "MQTT Client Shutdown in progress, aborting appstack initialisation routine.");
            return;
        }

        Print("MQTT Client", "PC Task ready! Setting PC Called flag!");
        xEventGroupSetBits(mqtt_event_group, MQTT_EVENT_PCTASK_CALL);
    }
    
    //TODO: After AppStack
    //Start downstream queue management tasks
    //Trigger app launch

    Print("MQTT Client", "Post Connection Task currently empty, but we've passed where successful execution would make it! Exiting.");

    xEventGroupSetBits(mqtt_event_group, MQTT_EVENT_PCTASK_DONE);
    xPostConnectionTask = NULL;
    vTaskDelete(NULL);
    return;
}

static int _reconnection_attempts = 0;

static void ConnectionEstablished(void)
{
    EventBits_t event_group_details;
    if(mqtt_event_group == NULL)
    {
        Print("MQTT Client", "Event group not initialized! Aborting connected call and terminating client! Invalid state!!!");
        ShutdownMQTTServices();
        return;
    }
    else
    {
        event_group_details = xEventGroupGetBits(mqtt_event_group);
        if(event_group_details & MQTT_EVENT_TERMINATING)
        {
            Print("MQTT Client", "MQTT Client Shutdown in progress, aborting startup routine.");
            return;
        }

        Print("MQTT Client", "Connection established! Setting service ready flag!");
        xEventGroupSetBits(mqtt_event_group, MQTT_EVENT_SERVICE_RDY);
    }

    _reconnection_attempts = 0;

    if(xPostConnectionTask != NULL)
    {
        Print("MQTT Client", "Post-connection task already running! Assuming reconnection event, ignoring.");
        return;
    }

    if(event_group_details & (MQTT_EVENT_PCTASK_CALL | MQTT_EVENT_PCTASK_DONE))
    {
        Print("MQTT Client", "Post-connection co-routine already called and/or ran! Ignoring.");
        return;
    }

    Print("MQTT Client", "Post-connection co-routine not yet called, starting it.");
    
    BaseType_t resultant = xTaskCreate(
        Post_Connection_Task,
        "MQTT PC Task",
        MQTT_PC_TASK_STACK_SIZE,
        NULL,
        MQTT_NODE_TASK_PRIORITY,
        &xPostConnectionTask
    );

    if(resultant != pdPASS)
    {
        //TODO: Handle this a bit better.
        Print("MQTT Client", "Post-connection task creation failed! Fatal error - shutting down MQTT services.");
        ShutdownMQTTServices();
    }
}

static void ConnectionLost(void)
{
    Print("MQTT Client", "Connection lost!");

    if(mqtt_event_group == NULL)
    {
        Print("MQTT Client", "Event group not initialized! Aborting disconnected routine and terminating client! Invalid state!!!");
        ShutdownMQTTServices();
        return;
    }

    EventBits_t event_group_details = xEventGroupClearBits(mqtt_event_group, MQTT_EVENT_SERVICE_RDY);

    if(event_group_details & MQTT_EVENT_TERMINATING)
    {
        Print("MQTT Client", "MQTT Service is being terminated, ignoring connection loss.");
        return;
    }

    if(_reconnection_attempts > MQTT_MAX_RECON_ATTEMPTS)
    {
        Print("MQTT Client", "Maximum reconnection attempts exceeded!");
        Print("MQTT Client", "Terminating MQTT Service.");

        InvalidateMQTTConfiguration();
        ShutdownMQTTServices();
    }

    _reconnection_attempts++;
    
    switch(esp_mqtt_client_reconnect(mqtt_client))
    {
        case ESP_OK:
            Print("MQTT Client", "Forced reconnection on client.");
            return;

        case ESP_ERR_INVALID_ARG:
            Print("MQTT Client", "Bad mqtt_client handle! Ignoring for now but this could potentially be a source of bugs!");
            break;

        case ESP_FAIL:
            Print("MQTT Client", "Client is in an invalid state! Ignoring for now but this could potentially be a source of bugs!");
            break;
        
        default:
            Print("MQTT Client", "Unhandled error! Please open a GitHub issue!");
            break;
    }
}

static void _mqtt_event_handler(
    void* handler_args, esp_event_base_t event_base,
    int32_t event_id, void* event_data
) {
    //if(event_base != MQTT_well I don't know.)
    //TODO: Find out what MQTT events bases are and reject any potential others.

    esp_mqtt_event_t *event = (esp_mqtt_event_t *)event_data;

    switch(event_id)
    {
		case MQTT_EVENT_ERROR:
            switch(event->error_handle->error_type)
            {
                case MQTT_ERROR_TYPE_NONE:
                    Print("MQTT Event Handler", "Erorr event occured yet no error type was supplied?");
                    Print("MQTT Event Handler", "Ignoring for now, take note of this if anything else is going wrong.");
                    break;

                case MQTT_ERROR_TYPE_TCP_TRANSPORT:
                    Print("MQTT Event Handler", "TCP Transport error occured. Ignoring.");
                    break;

                case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
                    switch(event->error_handle->connect_return_code)
                    {
                        case MQTT_CONNECTION_ACCEPTED:
                            Print("MQTT Event Handler", "Invalid state! Connection refused error contained accepted connection code!");
                            Print("MQTT Event Handler", "Please open a GitHub issue and attach the log file.");
                        	break;
                            
                        case MQTT_CONNECTION_REFUSE_PROTOCOL:
                            Print("MQTT Event Handler", "Connection refused because of a protocol error.");
                            Print("MQTT Event Handler", "Please check that the server you are trying to connect to is running mqtt!");
                        	InvalidateMQTTConfiguration();
                            ShutdownMQTTServices();
                            break;
                            
                        case MQTT_CONNECTION_REFUSE_ID_REJECTED:
                            Print("MQTT Event Handler", "Connection refused on the basis of client ID - please ensure this ID is not blocklisted.");
                            InvalidateMQTTConfiguration();
                            ShutdownMQTTServices();
                        	break;
                            
                        case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
                            Print("MQTT Event Handler", "Connection refused because the server is unavailable.");
                            Print("MQTT Event Handler", "Please check that the server you are trying to connect to is running mqtt!");
                            InvalidateMQTTConfiguration();
                            ShutdownMQTTServices();
                        	break;
                            
                        case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
                            Print("MQTT Event Handler", "Connection refused because of a bad username.");
                            Print("MQTT Event Handler", "MQTT-Node is not currently configured to support u/p connection option - feel free to extend yourself!");
                            InvalidateMQTTConfiguration();
                            ShutdownMQTTServices();
                        	break;
                            
                        case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
                            Print("MQTT Event Handler", "Connection refused because of a bad password.");
                            Print("MQTT Event Handler", "MQTT-Node is not currently configured to support u/p connection option - feel free to extend yourself!");
                            InvalidateMQTTConfiguration();
                            ShutdownMQTTServices();
                        	break;
                    }
                    break;

                default:
                    Print("MQTT Event Handler", "Unknown error type occured. Ignoring.");
                    break;
            }
			break;

		case MQTT_EVENT_CONNECTED:
            Print("MQTT Event Handler", "Connected to MQTT server.");
            Print("MQTT Event Handler", "Launching post connection task.");
            ConnectionEstablished();
			break;

		case MQTT_EVENT_DISCONNECTED:
            Print("MQTT Event Handler", "Connection lost!");
            Print("MQTT Event Handler", "Clearing flags to halt reliant processes");
            ConnectionLost();
			break;
            
		case MQTT_EVENT_SUBSCRIBED:
			break;
            
		case MQTT_EVENT_UNSUBSCRIBED:
			break;
            
		case MQTT_EVENT_PUBLISHED:
			break;
            
        //TODO: After AppStack
		case MQTT_EVENT_DATA:
			break;

		case MQTT_EVENT_BEFORE_CONNECT:
			break;
            
		case MQTT_EVENT_DELETED:
			break;
    }
}

static void CleanMQTTClient(void)
{
    Print("MQTT Cleanup Routine", "Cleanup requested!");

    if(mqtt_client != NULL)
    {
        Print("MQTT Cleanup Routine", "Client Handle exists! Cleaning...");
        switch(esp_mqtt_client_stop(mqtt_client))
        {
            case ESP_OK:
                Print("MQTT Cleanup Routine", "Client Handle stopped!");
                break;

            case ESP_ERR_INVALID_STATE:
                Print("MQTT Cleanup Routine", "Client Handle not running!");
                break;

            default:
                Print("MQTT Cleanup Routine", "Client Handle stop failed! Continuing anyway.");
                break;
        }
        
        //TODO: Confirm this also destroys event handler.
        switch(esp_mqtt_client_destroy(mqtt_client))
        {
            case ESP_OK:
                Print("MQTT Cleanup Routine", "Client Handle destroyed!");
                break;

            case ESP_ERR_INVALID_STATE:
                Print("MQTT Cleanup Routine", "Client Handle not initialised!");
                break;
            
            default:
                Print("MQTT Cleanup Routine", "esp_mqtt_client_destroy returned unhandled code!");
                Print("MQTT Cleanup Routine", "Continuing - however this could be the source of any preceeding bugs!");
                break;
        }

        mqtt_client = NULL;
    }

    if(mqtt_event_group != NULL)
    {
        Print("MQTT Cleanup Routine", "Destroying event handler");
        vEventGroupDelete(mqtt_event_group);
        mqtt_event_group = NULL;
    }

    mqtt_callbacks.valid_connection     = NULL;
    mqtt_callbacks.unsuccessful         = NULL;
    mqtt_callbacks.ungraceful_shutdown  = NULL;

    Print("MQTT Cleanup Routine", "Cleanup complete!");
    return;
}

static TaskHandle_t xTerminationHandlerTask = NULL;
static void Termination_Handler(void *pvParameters)
{
    EventBits_t event_group_details;

    if(mqtt_event_group == NULL)
    {
        Print("MQTT Termination Handler", "Event group not initialised!");
        Print("MQTT Termination Handler", "Ignoring and continuing in case termination called due to unclean shutdown error retry request.");
        event_group_details = ~0;
    }
    else
    {
        Print("MQTT Termination Handler", "Setting the termination event flag!");
        event_group_details = xEventGroupGetBits(mqtt_event_group);
        if(event_group_details & MQTT_EVENT_TERMINATING)
        {
            Print("MQTT Termination Handler", "Termination event already set!");
            Print("MQTT Termination Handler", "Termination flag already set! Assuming erroneous retry request and ignoring. Please clear termination flag before calling again upon unclean shutdown cleanup.");
            xTerminationHandlerTask = NULL;
            vTaskDelete(NULL);
            return;
        }
        else
        {
            xEventGroupSetBits(mqtt_event_group, MQTT_EVENT_TERMINATING);
        }
    }
    
    //TODO: After AppStack
    if(event_group_details & MQTT_EVENT_SERVICE_RDY)
    {
        Print("MQTT Termination Handler", "MQTT Server connection established.");
        if(event_group_details & MQTT_EVENT_PCTASK_CALL)
        {
            Print("MQTT Termination Handler", "MQTT Appstack Initialiser Called");
            if(event_group_details & MQTT_EVENT_PCTASK_DONE)
            {
                Print("MQTT Termination Handler", "MQTT Appstack Initialiser Complete");
                //Destroy downstream services.
            }
            else
            {
                Print("MQTT Termination Handler", "MQTT Appstack Initialiser not completed!");
                //Check if the task exists - wait for it and then kill it gracefully.
            }
        }
    }

    CleanMQTTClient();

    Print("MQTT Termination Handler", "Cleanup complete! Killing self.");
    xTerminationHandlerTask = NULL;
    vTaskDelete(NULL);
    return;
}

void ShutdownMQTTServices(void)
{
    Print("MQTT Shutdown", "Shutdown requested!");

    if(xTerminationHandlerTask != NULL)
    {
        Print("MQTT Shutdown", "Shutdown task already running! Ignoring extraneous call.");
        return;
    }

    BaseType_t resultant = xTaskCreate(
        Termination_Handler,
        "MQTT Shutdown",
        MQTT_STOP_STACK_SIZE,
        NULL,
        MQTT_NODE_TASK_PRIORITY,
        &xTerminationHandlerTask
    );

    if(resultant != pdPASS)
    {
        Print("MQTT Shutdown", "Failed to create shutdown task! Aborting..."); //Should we retry??
        return;
    }

    Print("MQTT Shutdown", "Shutdown task launched!");
}

int InitialiseMQTTClient(void)
{
    if(mqtt_client != NULL)
    {
        Print("MQTT Initialiser", "MQTT Client Handle currently occupied!");
        return MQTT_START_DUPLICATE;
    }

    Print("MQTT Initialiser", "Initialising Client");
    mqtt_client = esp_mqtt_client_init(&mqtt_configuration);

    if(mqtt_client == NULL)
    {
        Print("MQTT Initialiser", "esp_mqtt_client_init failed!");
        return MQTT_START_FAILURE;
    }

    switch(esp_mqtt_client_register_event(
        mqtt_client, ESP_EVENT_ANY_ID,
        _mqtt_event_handler, NULL
    )) {
        case ESP_OK:
            Print("MQTT Initialiser", "Successfully attached mqtt event handler");
            break;

        case ESP_ERR_NO_MEM:
            Print("MQTT Initialiser", "Not enough memory to attach event handler!");
            return MQTT_START_FAILURE;

        case ESP_ERR_INVALID_ARG:
            Print("MQTT Initialiser", "Invalid argument passed to event registration!");
            return MQTT_START_FAILURE;
    }

    Print("MQTT Initialiser", "Ensuring station has aquired IP...");
    _blockfor_hasip();

    switch(esp_mqtt_client_start(mqtt_client))
    {
        case ESP_OK:
            Print("MQTT Initialiser", "Successfully started MQTT Client");
            break;

        case ESP_ERR_INVALID_ARG:
            Print("MQTT Initialiser", "Invalid argument passed to esp_mqtt_client_start!");
            return MQTT_START_FAILURE;

        case ESP_FAIL:
            Print("MQTT Initialiser", "esp_mqtt_client_start failed for unknown reason!");
            return MQTT_START_FAILURE;
    }

    Print("MQTT Initialiser", "Successfully initialised MQTT Client");
    Print("MQTT Initialiser", "IN/OUT Tasks will be started upon successful connection");
    return MQTT_START_SUCCESS;
}

static TaskHandle_t xMQTTInitialiseHandle = NULL;
void Initialise_Handler(void* pvParameters)
{
    switch(InitialiseMQTTClient())
    {
        case MQTT_START_SUCCESS:
            Print("MQTT Initialiser", "MQTT Service Initialised, exiting startup task.");
            break;

        case MQTT_START_DUPLICATE:
            Print("MQTT Initialiser", "WARNING: MQTT Client already exists!");
            Print("MQTT Initialiser", "If there is no MQTT connection - perhaps a faulty termination is to blame.");
            Print("MQTT Initialiser", "Ignoring for now - but check to see who's calling init if there's a bug you can't find, likely arises there.");
            break;

        case MQTT_START_FAILURE:
            Print("MQTT Initialiser", "MQTT Client Initialisation failed!");
            Print("MQTT Initialiser", "Cleaning up...");
            CleanMQTTClient();
            break;
    }

    xMQTTInitialiseHandle = NULL;
    vTaskDelete(NULL);
}

int StartMQTTClient(
    void (*valid_connection)(void),
    void (*unsuccessful)(void),
    void (*ungraceful_shutdown)(void)
) {
    if(xMQTTInitialiseHandle != NULL)
    {
        Print("MQTT Client", "Initialisation task already running!");
        Print("MQTT Client", "Request ignored...");
        return MQTT_START_DUPLICATE;
    }

    if(mqtt_event_group != NULL)
    {
        Print("MQTT Client", "Event group already exists!");
        Print("MQTT Client", "Either a faulty termination is to blame or you're trying to start the client twice!");
        return MQTT_START_DUPLICATE;
    }

    mqtt_event_group = xEventGroupCreate();

    mqtt_callbacks.valid_connection     = valid_connection;
    mqtt_callbacks.unsuccessful         = unsuccessful;
    mqtt_callbacks.ungraceful_shutdown  = ungraceful_shutdown;

    _generate_mqtt_client_configuration();

    Print("MQTT Client", "Initialisation request - creating initialisation handler to dissociate from calling task...");
    xTaskCreate(
        Initialise_Handler,
        "MQTT-Start",
        MQTT_START_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY,
        &xMQTTInitialiseHandle
    );

    return MQTT_START_SUCCESS;
}