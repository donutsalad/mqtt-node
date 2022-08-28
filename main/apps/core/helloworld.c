#include "app_manifest.h"

int HelloWorld_Main(unsigned char *id, hash_t data_tag, char *data, size_t data_len)
{
    return 0;
}

void HelloWorld_Incoming(unsigned char id, mqtt_request_t* buffer)
{
    Print("Hello World App", "Message recieved loud and clear!");
    FreeIncomingBufferBlock(buffer);
}

void HelloWorld_Pause(unsigned char id)
{

}

void HelloWorld_Resume(unsigned char id)
{

}

void HelloWorld_Destroy(unsigned char id)
{

}

void HelloWorld_Cleanup()
{

}

size_t HelloWorld_ID_Scan(unsigned char *ids, size_t max_ids)
{
    //TODO: Make this a little more logical with return codes.
    if(max_ids > 1) return 0;
    ids[0] = 0;
    return 1;
}

app_status_t HelloWorld_Status(unsigned char id)
{
    return APP_STATUS_READY;
}