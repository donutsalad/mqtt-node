#include "mqttinterface.h"

#include <string.h>

#define BUFFER_ZERO 0
#define BUFFER_USED 1
#define BUFFER_FREE 2

typedef struct BUFFER_WRAPPER_INCOMING {
    unsigned char used;
    mqtt_request_t data;
} _incoming_t;

typedef struct BUFFER_WRAPPER_OUGOING {
    unsigned char used;
    mqtt_outgoing_t data;
} _outgoing_t;


//Could potentially eventually use xCreateQueueStatic to fully wrap in queues.
//Depends on wether or not post-queue-recieve buffer usage is commonplace
// - Deep copying would take up cycles saved by dequeuing a pointer and freeing the data later

//NOTE: If the buffer and queue are combined; you would have to rewrite buffer/queue architecture;
//      Currently the buffer for the data is requested seperately from queueing the data, hence, 
//      local buffer allocation calls, and subsequent logic would need to be tasked to the queue handler.
//      May not be worth the effort considering buffer requests are not always queued.

_incoming_t _incoming_messages_buffer[MQTT_INCOMING_REQUEST_QUEUE_LENGTH];
_outgoing_t _outgoing_messages_buffer[MQTT_OUTGOING_DATA_QUEUE_LENGTH];

//Tries to get a ZERO'd buffer from the list, otherwise zero's out a free entry and returns it
//Returns NULL when there are no free entries
mqtt_request_t* GetIncomingBufferBlock()
{
    int free_idx = -1;
    for(int i = 0; i < MQTT_INCOMING_REQUEST_QUEUE_LENGTH; i++)
    {
        if(_incoming_messages_buffer[i].used == BUFFER_ZERO)
        {
            _incoming_messages_buffer[i].used = BUFFER_USED;
            return &_incoming_messages_buffer[i].data;
        }
        else if(_incoming_messages_buffer[i].used == BUFFER_FREE)
        {
            if(free_idx == -1) free_idx = i;
        }
    }

    if(free_idx != -1)
    {
        memset(&_incoming_messages_buffer[free_idx].data, 0, sizeof(mqtt_request_t));
        _incoming_messages_buffer[free_idx].used = BUFFER_USED;
        return &_incoming_messages_buffer[free_idx].data;
    }

    return NULL;
}

//Pushes responsibility of freeing buffer block to cleanup task or future caller (when no zero entries found)
//Only use this when the caller is timepoor.
void FreeIncomingBufferBlock(mqtt_request_t* buffer)
{
    for(int i = 0; i < MQTT_INCOMING_REQUEST_QUEUE_LENGTH; i++)
    {
        if(&_incoming_messages_buffer[i].data == buffer)
        {
            _incoming_messages_buffer[i].used = BUFFER_FREE;
            return;
        }
    }
}

//Zero's out the buffer and free's it
//If time is of the essence you use FreeIncomingBufferBlock instead.
void ZeroIncomingBufferBlock(mqtt_request_t* buffer)
{
    for(int i = 0; i < MQTT_INCOMING_REQUEST_QUEUE_LENGTH; i++)
    {
        if(&_incoming_messages_buffer[i].data == buffer)
        {
            memset(&_incoming_messages_buffer[i].data, 0, sizeof(mqtt_request_t));
            _incoming_messages_buffer[i].used = BUFFER_ZERO;
            return;
        }
    }
}

//Tries to get a ZERO'd buffer from the list, otherwise zero's out a free entry and returns it
//Returns NULL when there are no free entries
mqtt_outgoing_t* GetOutgoingBufferBlock()
{
    int free_idx = -1;
    for(int i = 0; i < MQTT_OUTGOING_DATA_QUEUE_LENGTH; i++)
    {
        if(_outgoing_messages_buffer[i].used == BUFFER_ZERO)
        {
            _outgoing_messages_buffer[i].used = BUFFER_USED;
            return &_outgoing_messages_buffer[i].data;
        }
        else if(_outgoing_messages_buffer[i].used == BUFFER_FREE)
        {
            if(free_idx == -1) free_idx = i;
        }
    }

    if(free_idx != -1)
    {
        memset(&_outgoing_messages_buffer[free_idx].data, 0, sizeof(mqtt_outgoing_t));
        _outgoing_messages_buffer[free_idx].used = BUFFER_USED;
        return &_outgoing_messages_buffer[free_idx].data;
    }

    return NULL;
}

//Pushes responsibility of freeing buffer block to cleanup task or future caller (when no zero entries found)
//Only use this when the caller is timepoor.
void FreeOutgoingBufferBlock(mqtt_outgoing_t* buffer)
{
    for(int i = 0; i < MQTT_OUTGOING_DATA_QUEUE_LENGTH; i++)
    {
        if(&_outgoing_messages_buffer[i].data == buffer)
        {
            _outgoing_messages_buffer[i].used = BUFFER_FREE;
            return;
        }
    }
}

//Zero's out the buffer and free's it
//If time is of the essence you use FreeIncomingBufferBlock instead.
void ZeroOutgoingBufferBlock(mqtt_outgoing_t* buffer)
{
    for(int i = 0; i < MQTT_OUTGOING_DATA_QUEUE_LENGTH; i++)
    {
        if(&_outgoing_messages_buffer[i].data == buffer)
        {
            memset(&_outgoing_messages_buffer[i].data, 0, sizeof(mqtt_outgoing_t));
            _outgoing_messages_buffer[i].used = BUFFER_ZERO;
            return;
        }
    }
}

//ZERO's out any lazy free'd buffer blocks in the incoming buffer
void CleanupIncomingBuffer()
{
    for(int i = 0; i < MQTT_INCOMING_REQUEST_QUEUE_LENGTH; i++)
    {
        if(_incoming_messages_buffer[i].used == BUFFER_FREE)
        {
            memset(&_incoming_messages_buffer[i].data, 0, sizeof(mqtt_request_t));
            _incoming_messages_buffer[i].used = BUFFER_ZERO;
        }
    }
}

//ZERO's out the first lazily free'd buffer block in the incoming buffer
void CleanOneIncomingBuffer()
{
    for(int i = 0; i < MQTT_INCOMING_REQUEST_QUEUE_LENGTH; i++)
    {
        if(_incoming_messages_buffer[i].used == BUFFER_FREE)
        {
            memset(&_incoming_messages_buffer[i].data, 0, sizeof(mqtt_request_t));
            _incoming_messages_buffer[i].used = BUFFER_ZERO;
            return;
        }
    }
}

//ZERO's out any lazy free'd buffer blocks in the outgoing buffer
void CleanupOutgoingBuffer()
{
    for(int i = 0; i < MQTT_OUTGOING_DATA_QUEUE_LENGTH; i++)
    {
        if(_outgoing_messages_buffer[i].used == BUFFER_FREE)
        {
            memset(&_outgoing_messages_buffer[i].data, 0, sizeof(mqtt_outgoing_t));
            _outgoing_messages_buffer[i].used = BUFFER_ZERO;
        }
    }
}

//ZERO's out the first lazily free'd buffer block in the outgoing buffer
void CleanOneOutgoingBuffer()
{
    for(int i = 0; i < MQTT_OUTGOING_DATA_QUEUE_LENGTH; i++)
    {
        if(_outgoing_messages_buffer[i].used == BUFFER_FREE)
        {
            memset(&_outgoing_messages_buffer[i].data, 0, sizeof(mqtt_outgoing_t));
            _outgoing_messages_buffer[i].used = BUFFER_ZERO;
            return;
        }
    }
}

//ZERO's out all lazily free'd blocks in all buffers.
//This could take a while so only call this from a seperate task or when you're sure it'd be non-blocking.
void CleanAllBuffers()
{
//Minor speedup but worth the 10 lines or whatever it is.
#if MQTT_INCOMING_REQUEST_QUEUE_LENGTH == MQTT_OUTGOING_DATA_QUEUE_LENGTH
    for(int i = 0; i < MQTT_INCOMING_REQUEST_QUEUE_LENGTH; i++)
    {
        if(_incoming_messages_buffer[i].used == BUFFER_FREE)
        {
            memset(&_incoming_messages_buffer[i].data, 0, sizeof(mqtt_request_t));
            _incoming_messages_buffer[i].used = BUFFER_ZERO;
        }
        if(_outgoing_messages_buffer[i].used == BUFFER_FREE)
        {
            memset(&_outgoing_messages_buffer[i].data, 0, sizeof(mqtt_outgoing_t));
            _outgoing_messages_buffer[i].used = BUFFER_ZERO;
        }
    }
#else
    CleanupIncomingBuffer();
    CleanupOutgoingBuffer();
#endif
}