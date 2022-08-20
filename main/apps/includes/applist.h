#include "commons.h"
#include "mqttinterface.h"

#define APP_LIST_MAXIMUM_CONCURRENT_INSTANCES  15

#define MSG_DELIVERED           42
#define LIST_RESUMED            3
#define LIST_PAUSED             2
#define LIST_DESTROYED          1
#define LIST_INSTANTIATED       0
#define LIST_BAD_APP_NAME       -1
#define LIST_SINGLET_DUPLICANT  -2
#define LIST_NO_SPACE           -255

int Deliver_Message(mqtt_request_t* request);
int Create_New_Instance(
    hash_t stem_hash, char *stem,  size_t stem_len, 
    hash_t data_tag, char *data, size_t data_len
);