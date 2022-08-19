#include "mqttinterface.h"

#include <string.h>

/*-----------------------------------------------------------------------------------------------
 * Parses the MQTT message and returns MQTT_ENCODING_COMPLETE if successful.
 * If the message is split into multiple parts, the plan is to eventually handle that here,
 *  however currently it returns MQTT_ENCODING_NOT_IMPLEMENTED.
 * 
 * Expected Formats:
 * Topic: <client_id>(/<route>)?(/<path>)?(/stem[16])?
 *  - client_id     Leftmost part of the topic is always interpreted as client_id, and hashed
 *  - route         Hashed if present, if not, stem is cleared and route = path = MQTT_HASH_ROUTE_BASE
 *  - path          Hashed if present, if not, stem is cleared and path = MQTT_HASH_PATH_BASE
 *  - stem          If present; copied, truncated, into buffer->stem[16].
 * 
 * Payload: (<data_type>):<data>
 *  - data_type     Hashed into buffer->data_tag if present, else MQTT_HASH_DATA_TYPELESS
 *  - data          Copied into buffer->data[256], truncated if longer.
 * 
 * NOTE: If there is no colon in the payload, technically this is undefined behaviour,
 *       however currently we assume typeless data, and broadcast a warning over MQTT.
 *-----------------------------------------------------------------------------------------------*/
int ParseIncomingMQTTMessage(
    mqtt_request_t* buffer, int msg_id,
    int total_data_len, int current_data_offset,
    char *data, int data_len,
    char *topic, int topic_len
) {
    if(total_data_len < 0 || current_data_offset < 0 || data_len < 0 || topic_len < 0) return MQTT_ENCODING_INVALID_LENGTH;
    else if(total_data_len != data_len) {
        Print("MQTT Encoding", "Message is split into multiple packets! State is valid, however handling thereof is currently not implemented. Ignoring!");
        return MQTT_ENCODING_NOT_IMPLEMENTED;
    }

    //TODO: Handle split messages by handing off information to a background task and let that task handle the message building.
    //      Assuming it could be a while in some cases, that bifurcation should handle subsequent routing, this call should
    //      not block under any circumstances.
    //NOTE: MQTT_ENCODING_HANDOFF has been defined, please return this to let the caller know not to queue it's buffer.

    //Probably also split the below behaviour into static inlines to avoid bifurcating identical functionality.

    //Ensuring null termination to prevent upstream bourne overflow bugs from propagating.
    //topic[topic_len - 1] = '\0';
    //Turns out we can't edit this!

    char* idx = djb_hash_toslash(topic, &buffer->client_id);
    if(*idx == '\0')
    {
        //Let downstream code know there is no subroutes in the topic.
        buffer->route = buffer->path = MQTT_HASH_ROUTE_BASE;
        //Ensuring that the stem is clear.
        memset(buffer->stem, 0, sizeof(buffer->stem));

        goto DATA;
    }

    idx = djb_hash_toslash(topic, &buffer->route);
    if(*idx == '\0')
    {
        //Let downstream code know there is no path in the topic.
        buffer->path = MQTT_HASH_PATH_BASE;
        //Ensuring that the stem is clear.
        memset(buffer->stem, 0, sizeof(&buffer->stem));

        goto DATA;
    }

    idx = djb_hash_toslash(topic, &buffer->path);
    if(*idx == '\0')
    {
        //Ensuring that the stem is clear.
        memset(buffer->stem, 0, sizeof(buffer->stem));
    }
    else
    {
        //Stem is the remainder of the topic (truncated if longer)
        strncpy(buffer->stem, idx, sizeof(buffer->stem));

        //Prevent null termination error from nulltermless buffer.
        //(strncpy does not null terminate if the destination is smaller than the source)
        buffer->stem[sizeof(buffer->stem) - 1] = '\0';
    }

DATA:

    //Ensuring null termination to prevent upstream bourne overflow bugs from propagating.
    //data[data_len - 1] = '\0';
    //Turns out we can't edit this!

    //All data messages, including typeless payloads, will delimit with x:y where x is type and y is data.
    //Typeless payloads simply start with : in the form :y.
    //If there is no data x is assumed to be the data, but a warning is printed and broadcasted.
    idx = strchr(data, ':');
    if(idx == NULL)
    {
        Print("MQTT Encoding", "No data delimiter found in message! Assuming data is the entire message.");
        //TODO: Queue a MQTT Client message to make sure the caller knows we expect :y, but have accepted the request anyway.

        buffer->data_tag = MQTT_HASH_DATA_TYPELESS;
        strncpy(buffer->data, data, sizeof(buffer->data));
    }
    else if(idx == data)
    {
        buffer->data_tag = MQTT_HASH_DATA_TYPELESS;
        strncpy(buffer->data, ++idx, sizeof(buffer->data)); //idx is ':', so pre-increment to get to the data.
    }
    else
    {
        djb_hash_tocolon(data, &buffer->data_tag);
        strncpy(buffer->data, ++idx, sizeof(buffer->data)); //Ditto above.
    }

    //Prevent null termination error from nulltermless buffer.
    //(strncpy does not null terminate if the destination is smaller than the source)
    buffer->data[sizeof(buffer->data) - 1] = '\0';
    
    return MQTT_ENCODING_COMPLETE;
}